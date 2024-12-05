/*
 * adda_wandler.h
 *
 * Created: 17.10.2023 00:24:00
 *  Author: bx541002
 */ 


#ifndef ADDA_WANDLER_H_
#define ADDA_WANDLER_H_

// Controls the R-2R Network
void manuell(void);

// Converts an analog signal to a digital signal using the successive approximation converter
void sar(void);

// Converts an analog signal to a digital signal using the the tracking converter
void tracking(void);

#endif /* ADDA_WANDLER_H_ */