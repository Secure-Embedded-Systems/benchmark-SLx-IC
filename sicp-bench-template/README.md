# Demonstrate and Test Strategies for Securing MSP430 Operation During Powerloss

Use the [msp430-ctpl-gcc](https://github.com/Secure-Embedded-Systems/msp430-ctpl-gcc) library to enable security operations on detected powerloss.

## Approach
1. On power-loss:
	1. Compute MAC (Message Authentication Code) over the saved CTPL State on powerloss.
	2. Encrypt "Secure Storage" area for confidential information.
2. On wake-up:
	1. Recompute MAC and compared with stored value -- check for tampering.
	2. Check CTPL State and identify replay attacks.
	3. Decrypt "Secure Storage" area to complete state restoration



