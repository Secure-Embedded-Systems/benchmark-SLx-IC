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

#ifndef SIC_H
#define SIC_H

/* msp430 includes */
#include <msp430.h>
#include <stdlib.h>
#include <string.h>

/* for rng support */
#include "rng.h"

/* for EAX support */
#include "aes.h"
#include "modes.h"

/* 
 * Defines for SIC functions
 */
//#define SIC_SECURE_MEM_LEN			0x1000	/* 4096 B */

//#define SIC_SECURE_MEM_LEN_WORDS	0x0800	/* SIC_SECURE_MEM_LEN / 2 */
#define SIC_SECURE_MEM_START		0x10000	/* must match linker file */
#define SIC_NONSEC_MEM_START        0x11000 /* must match linked file */

/*
 * Comment FIXED_MEM_LEN for customized checkpoint operation
 */
//#define FIXED_MEM_LEN
#define SIC_SECURE_MEM_LEN_WORDS    0x0400  /* SIC_SECURE_MEM_LEN / 2 */
#define SIC_SECURE_MEM_LEN          0x0800  /* 2048 B */
#define SIC_NONSEC_MEM_LEN_WORDS    0x0400
#define SIC_NONSEC_MEM_LEN    0x0800

/*
 * SIC crypto engines
 */
#define SIC_ENGINE_EAX			0
#define SIC_ENGINE_KETJESR		1
#define SIC_ENGINE_GIFT         2
#define SIC_ENGINE_ASCON        3

/* enable HW primitive support if available */
//#define CF_USE_HW_AES
#define CF_USE_DMA_HW_AES

/* Choose active sic engine here */
#define SIC_ENGINE SIC_ENGINE_EAX//GIFT //SIC_ENGINE_EAX

/* Choose location of checkpoint content*/
//#define NONSECURE_BENCH //comment to place benchmark vars in secure mem
#define NONSECURE_PERPH //comment to place peripherals in secure mem

/*
 * Initialize
 * 
 * Called once to start the chain of tags to ensure a program's forward
 * progress.
 */
int sic_initialize(void);

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
int sic_refresh(void);

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
 * the previously valid state
 */
int sic_restore(void);

/*
 * Wipe
 *
 * Called each time the device loses power.
 *
 * Deletes volatile memory and current State data.
 * A sic_restore is required to resume operation.
 */
int sic_wipe(void);

#endif
