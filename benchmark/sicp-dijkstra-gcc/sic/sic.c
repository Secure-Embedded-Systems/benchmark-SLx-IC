/*
 * sic - secure intermittent computing
 * Written in 2018 by Charles Suslowicz <cesuslow@vt.edu>
 * Modified in 2021 by Archanaa S Krishnan <archanaa@vt.edu>
 *
 * To the extent possible under law, the author(s) have dedicated all
 * copyright and related and neighboring rights to this software to the
 * public domain worldwide. This software is distributed without any
 * warranty.
 *
 * You should have received a copy of the CC0 Public Domain Dedication
 * along with this software. If not, see
 * <http://creativecommons.org/publicdomain/zero/1.0/>.
 */

#include "sic.h"


/* For clarity */
#define PERSISTENT __attribute__ ((persistent))
#define SECURE __attribute__ ((section (".secure")))
#define TAMPERFREE __attribute__ ((section (".tamperfree")))

#define FLAG1  1

/* 
 * Defines to support different crypto engines
 */
#if SIC_ENGINE == SIC_ENGINE_EAX
	#define SIC_R_LEN		16
	#define SIC_TAG_LEN		16
	#define SIC_S_LEN		SIC_SECURE_MEM_LEN
#elif (SIC_ENGINE == SIC_ENGINE_ASCON) || (SIC_ENGINE == SIC_ENGINE_GIFT)
	#define SIC_R_LEN		31
	#define SIC_TAG_LEN		16
	#define SIC_S_LEN		SIC_SECURE_MEM_LEN+SIC_TAG_LEN	/* allows room for tags, created in crypto_aead_calls */
#endif


/*
 * Persistent Data for SIC
 */
static const uint8_t sic_key[16] = {0x00, 0x01, 0x02, 0x03,
									0x04, 0x05, 0x06, 0x07,
									0x08, 0x09, 0x0a, 0x0b,
									0x0c, 0x0d, 0x0e, 0x0f };

uint8_t * sec_state = (uint8_t *) SIC_SECURE_MEM_START;
#ifdef FIXED_MEM_LEN
static const uint16_t sec_state_len = SIC_SECURE_MEM_LEN;
#else
uint16_t sec_state_len = 0;
#endif

uint8_t * nonsec_state = (uint8_t *) SIC_NONSEC_MEM_START;
#ifdef FIXED_MEM_LEN
static const uint16_t nonsec_state_len = SIC_NONSEC_MEM_LEN;
#else
uint16_t nonsec_state_len = 0;
#endif
static const uint8_t empty_mem[32] = {   0xFF, 0xFF, 0xFF, 0xFF,
                                                0xFF, 0xFF, 0xFF, 0xFF,
                                                0xFF, 0xFF, 0xFF, 0xFF,
                                                0xFF, 0xFF, 0xFF, 0xFF,
                                                0xFF, 0xFF, 0xFF, 0xFF,
                                                0xFF, 0xFF, 0xFF, 0xFF,
                                                0xFF, 0xFF, 0xFF, 0xFF,
                                                0xFF, 0xFF, 0xFF, 0xFF,};

PERSISTENT uint8_t r_a[SIC_R_LEN];
PERSISTENT uint8_t r_b[SIC_R_LEN];
PERSISTENT uint8_t tag_a[SIC_TAG_LEN];
PERSISTENT uint8_t tag_b[SIC_TAG_LEN];
PERSISTENT uint8_t tag_tmp[SIC_TAG_LEN];
PERSISTENT uint8_t s_a[SIC_S_LEN];
PERSISTENT uint8_t s_b[SIC_S_LEN];
PERSISTENT uint8_t s_tmp[SIC_S_LEN];
PERSISTENT uint8_t ad[SIC_SECURE_MEM_LEN];
PERSISTENT uint8_t ad_len;
uint16_t tag_b_len = 0;
TAMPERFREE struct flags
{
     unsigned char flag_a :1;
     unsigned char flag_b :1;
}f;
//SECURE unsigned char flag_a        :1;
//SECURE unsigned char flag_a        :1;
#if SIC_ENGINE == SIC_ENGINE_EAX
TAMPERFREE cf_aes_context sic_ctx;
#endif

/*
 * SIC functions
 */
void sic_incCounter(uint8_t *counter, size_t counter_len);		/* Increments counter */
uint16_t sic_generateBytes(uint8_t *dst, uint16_t length);		/* Wraps rng_generateBytes */
uint16_t sic_copyTag(uint16_t *dst, uint16_t *src, size_t len);

/*
 * Initialize
 * 
 * Called once to start the chain of tags to ensure a program's forward
 * progress.
 */
int sic_initialize(void)
{
	uint16_t ret_val, i;
	
	ctpl_checkpointOnly();
    //resest all flags
    f.flag_a &= ~FLAG1;
    f.flag_b &= ~FLAG1;

	/* initialize counters for system */
	memset(r_a, 0, sizeof(r_a));	
	memset(r_b, 0, sizeof(r_b));	

	/* T_B <- random() */
	tag_b_len = sizeof(tag_b);
	ret_val = sic_generateBytes(tag_b, tag_b_len);
	if (ret_val != tag_b_len) {
		return 1;
	}
//	memset(tag_b, 0, sizeof(tag_b));

	/* STATE <- 0 */
//	memset(state, 0, state_len);

	/*
	 * Identifying size of secure section
	 */
	for (i = 0; i < SIC_SECURE_MEM_LEN-32; i++)
	{
	    ret_val = memcmp(sec_state+i, empty_mem,32);
	    if(!ret_val)
	    {
	        sec_state_len = i;
	        break;
	    }
	}

	for (i = 0; i < SIC_NONSEC_MEM_LEN-32; i++)
	{
	    ret_val = memcmp(nonsec_state+i, empty_mem, 32);
	    if(!ret_val)
	    {
	        nonsec_state_len = i;
	        break;
	    }
	}
    /* Creating associated data */
    memcpy(ad, tag_b, tag_b_len);
    memcpy(ad+tag_b_len, nonsec_state, nonsec_state_len);
    ad_len = tag_b_len+nonsec_state_len;

#if SIC_ENGINE == SIC_ENGINE_EAX
	/* set key properly */
	cf_aes_init(&sic_ctx, sic_key, sizeof(sic_key));
	
	/* S_A <- AEAD(STATE, T_B, R_A, K) */
	/* T_A <- AEAD_Auth(S_A, T_B, R_A, K) */
	cf_eax_encrypt( &cf_aes, &sic_ctx,		/* eax state info */
					sec_state, sec_state_len,		/* input message (PT) */
					ad, ad_len,	/* associated data (chaining tag) */
					r_a, sizeof(r_a),		/* nonce / public number */
					s_a,					/* cipher text buffer */
					tag_tmp, sizeof(tag_tmp));	/* computed tag on cipher text */
	/* copy tag from tmp buffer */
	sic_copyTag(tag_a, tag_tmp, sizeof(tag_a));
	sic_setFlagA();


#elif (SIC_ENGINE == SIC_ENGINE_GIFT) || (SIC_ENGINE == SIC_ENGINE_ASCON)
	/* S_A <- AEAD(STATE, T_B, R_A, K) */
	/* T_A <- AEAD_Auth(S_A, T_B, R_A, K) */
	unsigned long long len_s_a;
	crypto_aead_encrypt(s_a, &len_s_a,
						sec_state, sec_state_len,
						ad, ad_len,
						0,						/* secret number */
						r_a,
						sic_key);
	
	if (len_s_a == (sec_state_len + SIC_TAG_LEN))
		//memcpy(tag_a, s_a + SIC_SECURE_MEM_LEN, sizeof(tag_a));
		/* copy tag from tmp buffer */
		sic_copyTag(tag_a, s_a + SIC_SECURE_MEM_LEN, sizeof(tag_a));
	else
		memset(tag_a, 0, sizeof(tag_a));
	sic_setFlagA();

#endif
	return 0;
}

/*
 * Refresh
 *
 * Called each time the saved state needs to be updated.
 *
 * Authenticates previous tag 
 * Computes a new state save packet 
 * Updates state save packet in alternating slot
 *
 * NOTE: the tag MUST be the last item updated, as it invalidates
 * the previously valid state
 */
int sic_refresh(void)
{
	uint8_t q[SIC_R_LEN];
	uint16_t ret_val;
	
	/* Q <- random() */
//	ret_val = sic_generateBytes(q, sizeof(q));
//	if (ret_val != sizeof(q)) {
//		return 1;
//	}

#if SIC_ENGINE == SIC_ENGINE_EAX
	/* set key properly (necessary with HW AES) */
	cf_aes_init(&sic_ctx, sic_key, sizeof(sic_key));
	
	if(f.flag_a && ~(f.flag_b)) {

	    /* Creating associated data*/
	    memcpy(ad, tag_b, sizeof(tag_b));
	    memcpy(ad+sizeof(tag_b), nonsec_state, nonsec_state_len);
	    ad_len = sizeof(tag_b)+nonsec_state_len;

        /* if T_A = AEAD-Dec(S_A, T_B, R_A, K) then */
        ret_val = cf_eax_decrypt(	&cf_aes, &sic_ctx,		/* eax state info */
                                 	s_a, sec_state_len,
//                                    s_a, sizeof(s_a),		/* input ct */
                                    ad, ad_len,             /* aad for ct*/
//                                    tag_b, sizeof(tag_b),	/* aad for ct */
                                    r_a, sizeof(r_a),		/* none for ct */
                                    tag_a, sizeof(tag_a),	/* tag to be auth'd */
                                    s_tmp);					/* resulting pt */
        /* ret_val = 0 if auth succeeded */
        if (!ret_val)
        {
            /* R_B <- Q */
    //		memcpy(r_b, q, sizeof(q));
            memcpy(r_b, r_a, sizeof(r_b));
            sic_incCounter(r_b, sizeof(r_b));

            /* Creating associated data*/
            /* associated data (chaining tag) */
            memcpy(ad, tag_a, sizeof(tag_a));
            memcpy(ad+sizeof(tag_a), nonsec_state, nonsec_state_len);
            ad_len = sizeof(tag_a)+nonsec_state_len;

            /* S_B <- AEAD-enc(STATE, T_A, R_B, K) */
            /* T_B <- AEAD-auth(S_B, T_A, R_B, K) */
            cf_eax_encrypt( &cf_aes, &sic_ctx,			/* eax state info */
                            sec_state, sec_state_len,			/* input message (PT) */
                            ad, ad_len,                 /* associated data (chaining tag) */
//                            tag_a, sizeof(tag_a),		/* associated data (chaining tag) */
                            r_b, sizeof(r_b),			/* nonce / public number */
                            s_b,						/* cipher text buffer */
                            tag_tmp, sizeof(tag_tmp));	/* computed tag on cipher text */

            /* copy tag from tmp buffer */
            sic_copyTag(tag_b, tag_tmp, sizeof(tag_b));
            sic_setFlagB();

            /* cleanup then return */
            memset(s_tmp, 0, sizeof(s_tmp));
            return 0;
        }
        else
            return 1;
	}

	else if (~(f.flag_a) && f.flag_b ){

	    /* Creating associated data*/
        memcpy(ad, tag_a, sizeof(tag_a));
        memcpy(ad+sizeof(tag_a), nonsec_state, nonsec_state_len);
        ad_len = sizeof(tag_a)+nonsec_state_len;

        /* if T_B = AEAD-Dec(S_B, T_A, R_B, K) */
        ret_val = cf_eax_decrypt(	&cf_aes, &sic_ctx,
                                 	s_b, sec_state_len,
//                                    s_b, sizeof(s_b),
                                    ad, ad_len,         /* aad */
//                                    tag_a, sizeof(tag_a),
                                    r_b, sizeof(r_b),
                                    tag_b, sizeof(tag_b),
                                    s_tmp);
        /* ret_val = 0 if auth succeeded */
        if (!ret_val)
        {
            /* R_B <- Q */
    //		memcpy(r_a, q, sizeof(q));
            memcpy(r_a, r_b, sizeof(r_a));
            sic_incCounter(r_a, sizeof(r_a));

            /* Creating associated data*/
            /* associated data (chaining tag) */
            memcpy(ad, tag_b, sizeof(tag_b));
            memcpy(ad+sizeof(tag_b), nonsec_state, nonsec_state_len);
            ad_len = sizeof(tag_b)+nonsec_state_len;


            /* S_B <- AEAD-enc(STATE, T_A, R_B, K) */
            /* T_B <- AEAD-auth(S_B, T_A, R_B, K) */
            cf_eax_encrypt( &cf_aes, &sic_ctx,			/* eax state info */
                            sec_state, sec_state_len,			/* input message (PT) */
                            ad, ad_len,
//                            tag_b, sizeof(tag_b),		/* associated data (chaining tag) */
                            r_a, sizeof(r_a),			/* nonce / public number */
                            s_a,						/* cipher text buffer */
                            tag_tmp, sizeof(tag_tmp));	/* computed tag on cipher text */

            /* copy tag from tmp buffer */
            sic_copyTag(tag_a, tag_tmp, sizeof(tag_a));
            sic_setFlagA();

            /* cleanup then return */
            memset(s_tmp, 0, sizeof(s_tmp));
            return 0;
        }
        else
            return 1;
	}
#elif SIC_ENGINE == SIC_ENGINE_KETJESR
	/* if T_A = AEAD-Dec(S_A, T_B, R_A, K) then */
	unsigned long long s_tmp_len;
	if ( crypto_aead_decrypt(s_tmp, &s_tmp_len,    /* message */
							0,                     /* secret number */
							s_a, sizeof(s_a),      /* ciphertext */
							tag_b, sizeof(tag_b),  /* associated data (chaining tag) */
							r_a,                   /* nonce */
							sic_key) == 0) {
		/* auth succeeded */
		
		/* R_B <- Q */
//		memcpy(r_b, q, sizeof(q));
		memcpy(r_b, r_a, sizeof(r_b));
		sic_incCounter(r_b, sizeof(r_b));
		
		/* S_B <- AEAD-enc(STATE, T_A, R_B, K) */
		/* T_B <- AEAD-auth(S_B, T_A, R_B, K) */
		unsigned long long len_s_b;
		crypto_aead_encrypt(s_b, &len_s_b,          /* cipher text buffer */
							sec_state, sec_state_len,   /* input message (PT) */
							tag_a, sizeof(tag_a),   /* associated data (chaining tag) */
							0,                      /* secret number */
							r_b,                    /* nonce / public number */
							sic_key);
		
		if (len_s_b == (sec_state_len + SIC_TAG_LEN))
			//memcpy(tag_b, s_b + SIC_SECURE_MEM_LEN, sizeof(tag_b));
			/* copy tag from tmp buffer */
			sic_copyTag(tag_b, s_b + SIC_SECURE_MEM_LEN, sizeof(tag_b));
		else
			memset(tag_b, 0, sizeof(tag_b));
		
		/* cleanup then return */
		memset(s_tmp, 0, sizeof(s_tmp));	
		return 0;
	}
  
	/* if T_B = AEAD-Dec(S_B, T_A, R_B, K) */
	if ( crypto_aead_decrypt(s_tmp, &s_tmp_len,
							0, /* secret number */
							s_b, sizeof(s_b),
							tag_a, sizeof(tag_a),
							r_b,
							sic_key) == 0) {
		/* auth succeeded */
	    
		/* R_A <- Q */
//		memcpy(r_a, q, sizeof(q));
		memcpy(r_a, r_b, sizeof(r_a));
		sic_incCounter(r_a, sizeof(r_a));
		
		/* S_B <- AEAD-enc(STATE, T_A, R_B, K) */
		/* T_B <- AEAD-auth(S_B, T_A, R_B, K) */
		unsigned long long len_s_a;
		crypto_aead_encrypt(s_a, &len_s_a,          /* cipher text buffer */
							sec_state, sec_state_len,   /* input message (PT) */
							tag_b, sizeof(tag_b),   /* associated data (chaining tag) */
							0,                      /* secret number */
							r_a,                    /* nonce / public number */
							sic_key);
		
		if (len_s_a == (sec_state_len + SIC_TAG_LEN))
			//memcpy(tag_a, s_a + SIC_SECURE_MEM_LEN, sizeof(tag_a));
			/* copy tag from tmp buffer */
			sic_copyTag(tag_a, s_a + SIC_SECURE_MEM_LEN, sizeof(tag_a));
		else
			memset(tag_a, 0, sizeof(tag_a));
		    
		/* cleanup then return */
		memset(s_tmp, 0, sizeof(s_tmp));	
		return 0;
	}

#elif SIC_ENGINE == SIC_ENGINE_ASCON
        return 10;
#endif

	/* If here, both authentications have failed.  Abort */
	memset(s_tmp, 0, sizeof(s_tmp));	
	return 1;
}

/*
 * Restore
 *
 * Called each time the saved state needs to be restored.
 *
 * Authenticates previous tag 
 * Decryptes Current state if authentication successful
 * Computes a new state save packet 
 * Updates state save packet in alternating slot
 *
 * NOTE: the tag MUST be the last item updated, as it invalidates
 * the previously valid tag
 */
int sic_restore(void)
{
	uint8_t q[SIC_R_LEN];
	uint16_t ret_val;
	
	/* Q <- random() */
//	ret_val = sic_generateBytes(q, sizeof(q));
//	if (ret_val != sizeof(q)) {
//		return 1;
//	}

#if SIC_ENGINE == SIC_ENGINE_EAX
	/* set key properly (necessary with HW AES) */
	cf_aes_init(&sic_ctx, sic_key, sizeof(sic_key));

	if(f.flag_a && ~(f.flag_b)) {
        /* Creating associated data*/
        memcpy(ad, tag_b, sizeof(tag_b));
        memcpy(ad+sizeof(tag_b), nonsec_state, nonsec_state_len);
        ad_len = sizeof(tag_b)+nonsec_state_len;
	/* if T_A = AEAD-Dec(S_A, T_B, R_A, K) then */
        ret_val = cf_eax_decrypt(	&cf_aes, &sic_ctx,		/* eax state info */
                                 	s_a, sec_state_len,
//                                    s_a, sizeof(s_a),		/* input ct */
                                    ad, ad_len,
//                                    tag_b, sizeof(tag_b),	/* aad for ct */
                                    r_a, sizeof(r_a),		/* none for ct */
                                    tag_a, sizeof(tag_a),	/* tag to be auth'd */
                                    sec_state);
        /* ret_val = 0 if auth succeeded */
        if (!ret_val) {
            return 0;
        }
	}
	else if(~(f.flag_a) && f.flag_b) {
	    /* Creating associated data*/
        memcpy(ad, tag_a, sizeof(tag_a));
        memcpy(ad+sizeof(tag_a), nonsec_state, nonsec_state_len);
        ad_len = sizeof(tag_a)+nonsec_state_len;
	
        /* if T_B = AEAD-Dec(S_B, T_A, R_B, K) */
        ret_val = cf_eax_decrypt(	&cf_aes, &sic_ctx,
                                 	s_b, sec_state_len,
//								s_b, sizeof(s_b),
								ad, ad_len, /*AAD*/
//								tag_a, sizeof(tag_a),
								r_b, sizeof(r_b),
								tag_b, sizeof(tag_b),
								sec_state);
        /* ret_val = 0 if auth succeeded */
        if (!ret_val) {
            return 0;
	}
	}

#elif SIC_ENGINE == SIC_ENGINE_KETJESR
	/* if T_A = AEAD-Dec(S_A, T_B, R_A, K) then */
	unsigned long long s_tmp_len;
	if ( crypto_aead_decrypt(sec_state, &s_tmp_len,    /* ciphertext */
							0,                     /* secret number */
							s_a, sizeof(s_a),      /* message */
							tag_b, sizeof(tag_b),  /* associated data (chainint tag) */
							r_a,                   /* nonce */
							sic_key) == 0) {
		/* auth succeeded */
		
		/* R_B <- Q */
//		memcpy(r_b, q, sizeof(q));
		memcpy(r_b, r_a, sizeof(r_b));
		sic_incCounter(r_b, sizeof(r_b));
		
		/* S_B <- AEAD-enc(STATE, T_A, R_B, K) */
		/* T_B <- AEAD-auth(S_B, T_A, R_B, K) */
		unsigned long long len_s_b;
		crypto_aead_encrypt(s_b, &len_s_b,          /* cipher text buffer */
							sec_state, sec_state_len,   /* input message (PT) */
							tag_a, sizeof(tag_a),   /* associated data (chaining tag) */
							0,                      /* secret number */
							r_b,                    /* nonce / public number */
							sic_key);
		
		if (len_s_b == (sec_state_len + SIC_TAG_LEN))
			//memcpy(tag_b, s_b + SIC_SECURE_MEM_LEN, sizeof(tag_b));
			/* copy tag from tmp buffer */
			sic_copyTag(tag_b, s_b + SIC_SECURE_MEM_LEN, sizeof(tag_b));
		else
			memset(tag_b, 0, sizeof(tag_b));
		
		return 0;
	}


	/* if T_B = AEAD-Dec(S_B, T_A, R_B, K) */
	if ( crypto_aead_decrypt(sec_state, &s_tmp_len,
						   0, /* secret number */
						   s_b, sizeof(s_b),
						   tag_a, sizeof(tag_a),
						   r_b,
						   sic_key) == 0) {
		/* auth succeeded */
		
		/* R_A <- Q */
//		memcpy(r_a, q, sizeof(q));
		memcpy(r_a, r_b, sizeof(r_a));
		sic_incCounter(r_a, sizeof(r_a));
		
		/* S_B <- AEAD-enc(STATE, T_A, R_B, K) */
		/* T_B <- AEAD-auth(S_B, T_A, R_B, K) */
		unsigned long long len_s_a;
		crypto_aead_encrypt(s_a, &len_s_a,          /* cipher text buffer */
							sec_state, sec_state_len,   /* input message (PT) */
							tag_b, sizeof(tag_b),   /* associated data (chaining tag) */
							0,                      /* secret number */
							r_a,                    /* nonce / public number */
							sic_key);
		
		if (len_s_a == (sec_state_len + SIC_TAG_LEN))
			//memcpy(tag_a, s_a + SIC_SECURE_MEM_LEN, sizeof(tag_a));
			/* copy tag from tmp buffer */
			sic_copyTag(tag_a, s_a + SIC_SECURE_MEM_LEN, sizeof(tag_a));
		else
			memset(tag_a, 0, sizeof(tag_a));
		
		/* cleanup then return */
		memset(s_tmp, 0, sizeof(s_tmp));	
		return 0;
	}
#elif SIC_ENGINE == SIC_ENGINE_ASCON
        return 10;
#endif
	/* If here, both authentications have failed.  Abort */
	memset(sec_state, 0, sec_state_len);
	return 1;
}

/*
 * Wipe
 *
 * Called each time the device loses power.
 *
 * Deletes volatile memory and current State data.
 * A sic_restore is required to resume operation.
 */
int sic_wipe(void)
{
	/* developer wipe? zero everything? */
	memset(r_a, 0, sizeof(r_a));
	memset(r_b, 0, sizeof(r_b));
	memset(tag_a, 0, sizeof(tag_a));
	memset(tag_b, 0, sizeof(tag_b));
	memset(tag_tmp, 0, sizeof(tag_tmp));
	memset(s_a, 0, sizeof(s_a));
	memset(s_b, 0, sizeof(s_b));
	memset(sec_state, 0, sec_state_len);
#if SIC_ENGINE == SIC_ENGINE_EAX
	memset(&sic_ctx, 0, sizeof(sic_ctx));
#endif
	return 0;
}

uint16_t sic_generateBytes(uint8_t *dst, uint16_t length)
{
	uint16_t diff;
	uint16_t ret_val;
	
	/* The RNG can only produce bytes in multiples of RNG_KEYLEN
	 * so if the required nonce if different, we have to grab a few
	 * extra bytes, then copy over the ones needed
	 */
	diff = length % RNG_KEYLEN;
	if (diff != 0) {
		uint8_t tmp[SIC_R_LEN + (RNG_KEYLEN - diff)];
		ret_val = rng_generateBytes(tmp, sizeof(tmp));
		memcpy(dst, tmp, length);
		return length;
	}
	else {
		ret_val = rng_generateBytes(dst, length);
		return ret_val;
	}
}

/*
 * Copy a computed tag in an atomic fashion
 *
 * Interrupts are disabled to ensure that the copy completes even if power is lost
 */
uint16_t sic_copyTag(uint16_t *dst, uint16_t *src, size_t len)
{
	uint16_t interruptState;
	uint16_t ret_val;

	/* Globally disable interrupts for duration of copy */
	interruptState = __get_interrupt_state();
	__disable_interrupt();

	ret_val = memcpy(dst, src, len);

	/* Restore interrupt state */
	__set_interrupt_state(interruptState);

	return ret_val;
}

/*
 * Set flag for SS_A(valid) and reset flag for SS_B (invalid)
 *
 * Interrupt disabled to ensure that the copy completes even if power is lost
 */
void sic_setFlagA()
{
    uint16_t interruptState;
    //Set S_A as valid and S_B as invalid
    /* Globally disable interrupts for duration of copy */
    interruptState = __get_interrupt_state();
    __disable_interrupt();

    f.flag_a |= FLAG1;
    f.flag_b &= ~FLAG1;

    /* Restore interrupt state */
    __set_interrupt_state(interruptState);

}

/*
 * Set flag for SS_B(valid) and reset flag for SS_A(invalid)
 *
 * Interrupt disabled to ensure that the copy completes even if power is lost
 */
void sic_setFlagB()
{
    uint16_t interruptState;
    //Set S_A as valid and S_B as invalid
    /* Globally disable interrupts for duration of copy */
    interruptState = __get_interrupt_state();
    __disable_interrupt();

    f.flag_b |= FLAG1;
    f.flag_a &= ~FLAG1;

    /* Restore interrupt state */
    __set_interrupt_state(interruptState);

}

/* Increment a Counter
 *
 * Assumes counter LSB is counter[0] and MSB is counter[counter_len-1]
 *
 * Params:
 *	counter - pointer to an array of uint8_t representing a counter
 *	counter_len - length of counter in bytes
 */
void sic_incCounter(uint8_t *counter, size_t counter_len)
{
	uint16_t i;
	for(i = 0; i < counter_len; i++) {
		counter[i] = counter[i] + 1;
		if (counter[i] != 0) 
			break;
	}
}
