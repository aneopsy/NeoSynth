/* Includes ------------------------------------------------------------------*/
#include <cstdlib>
#include "main.h"
#include "wave.h"
#include "stm32f7xx_hal.h"
#include "usb_device.h"
#include "usbd_core.h"
#include "usbd_MIDI.h"
#include "usbd_midi_if.h"
	
/* Private typedef -----------------------------------------------------------*/
DAC_HandleTypeDef    			DacHandle;
DAC_ChannelConfTypeDef 		sConfig;
TIM_MasterConfigTypeDef 	sMasterConfig;
TIM_HandleTypeDef  				htim2, htim3, htim4, htim5, htim6, htim7, htim8, htim9;
TIM_HandleTypeDef  				htim10, htim11, htim12, htim13, htim14;
RCC_PeriphCLKInitTypeDef 	PeriphClkInitStruct;
RCC_ClkInitTypeDef 				RCC_ClkInitStruct;
RCC_OscInitTypeDef 				RCC_OscInitStruct;

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
int16_t 									delay_in = 0, delay_out = 500;
uint16_t 									counter[12], output, delaybuffer[1000], pwval, pwmval, modval, tmp;
uint8_t 									key, velocity, ctrl, data;
uint8_t 									wavesel, wavesel2, voicesel, velsel, pwm, mod, vcf, tun, det, sus, notepos, bend, param;
uint8_t 									paramvalue[256];
uint8_t 									voice = 0, n, i, j, y, r, c, d, e, f, g, h, k, m;
uint8_t 									chord, seqnote, triad2, triad3, triad4;
uint16_t 									lfo1rate, lfo1 = 63, lfo1cnt = 1, lfo1pol = 1;
uint16_t 									lfo2rate, lfo2 = 63, lfo2cnt = 1, lfo2pol = 1;
uint16_t 									lfo3rate, lfo3 = 63, lfo3cnt = 1, lfo3pol = 1;
int8_t 										transpose, scale;
const uint32_t 						pitchtbl[] = {
	16384,15464,14596,13777,13004,12274,11585,10935,10321,9742,9195,
	8679,8192,7732,7298,6889,6502,6137,5793,5468,5161,4871,4598,4340,
	4096,3866,3649,3444,3251,3069,2896,2734,2580,2435,2299,2170,
	2048,1933,1825,1722,1625,1534,1448,1367,1290,1218,1149,1085,
	1024,967,912,861,813,767,724,683,645,609,575,542,
	512,483,456,431,406,384,362,342,323,304,287,271,
	256,242,228,215,203,192,181,171,161,152,144,136,
	128,121,114,108 ,102,96,91,85,81,76,72,68,64
};
uint8_t 									chanstat[6] = {255,255,255,255,255,255};
uint8_t 									vcfadsr[6] = {0,0,0,0,0,0}, vcaadsr[6] = {0,0,0,0,0,0};
float 										channel[12] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
float 										vcfcutoff[6] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
float 										vcacutoff[6] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
float 										vcfkeyfollow[6] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
float 										envkeyfollow[6] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
float 										summer = 0.0f, pbend = 0.0f, modlvl = 0.0f, pwmlvl = 0.0f, vcflvl = 0.0f, vcfval = 0.0f;
float 										vcfkflvl, envkflvl, oscmix, vco, vcfreleaselvl = 0.000001f, vcareleaselvl = 0.000001f, resonance;
float 										vcfattackrate[6], vcfdecayrate[6], vcfsustainlvl[6], vcfreleaserate[6];
float 										vcfvellvl[6], vcavellvl[6], vcfenvlvl;
float 										vcfattack, vcfdecay, vcfsustain, vcfrelease;
float 										vcaattackrate[6], vcadecayrate[6], vcasustainlvl[6], vcareleaserate[6];
float 										vcaattack, vcadecay, vcasustain, vcarelease;

/**
  * @brief  Main program.
  * @param  None
  * @retval None
  */
int main(void)
{
	/* Enable the CPU Cache */
	CPU_CACHE_Enable();

	/* STM32F7xx HAL library initialization */
	HAL_Init();

	/* Configure the system clock to 216 MHz */
	SystemClock_Config();

	/* Initialize all GPIOs */
	MX_GPIO_Init();
	
	/* Initialize the USB device */
	MX_USB_DEVICE_Init();

	/* Configures the DAC for stereo output */
	DAC_Config();

	/* Prepare the oscillators for use */
	PrepOscs();
	
	/* Start Timer 9 interrupts to trigger the mixer routine */
	TIM9_Config(3);

	/* Populate the synth engines parameters with the default patch values */
	DefaultPatch();
	
	/* Turn on LED1 when all is init */
	HAL_GPIO_WritePin(GPIOB, LD1_Pin, GPIO_PIN_SET);
	intro_music();
	while (1)
	{
		if (HAL_GPIO_ReadPin(User_Blue_Button_GPIO_Port, User_Blue_Button_Pin)) {
			stop_all_note();
			SelfTest();
		}
		
	}
}

void intro_music(void)
{
	int h = 63;
	
	play_note(h,127);
	HAL_Delay(180);
	stop_note(h);
	
	play_note(h+2,127);
	HAL_Delay(180);
	stop_note(h+2);
	
	play_note(h,127);
	HAL_Delay(250);
	stop_note(h);
	
	play_note(h-1,127);
	HAL_Delay(600);
	stop_note(h-1);
}

void envelope(void)
{
	/* Process the pitch of the notes */
	ProcessPitch();
	
	/* Process the VCF/VCA ADSRs */
	ProcessADSRs();
}

void mixer(void)
{
	/* Process the waveforms for all voices */
	MakeSound();
	
	/* Process the final sound and play thru the DAC */
	SynthOutput();
	
	/* Update the LFOs */
	UpdateLFOs();
}

void PrepOscs(void)
{
	/* Cycle oscillator timers so there is no initial output spike */
	TIM14_Config(2048);
	TIM2_Config(2048);
	TIM3_Config(2048);
	TIM4_Config(2048);
	TIM5_Config(2048);
	TIM8_Config(2048);
	TIM6_Config(2048);
	TIM7_Config(2048);
	TIM10_Config(2048);
	TIM11_Config(2048);
	TIM12_Config(2048);
	TIM13_Config(2048);
	HAL_TIM_Base_Stop(&htim14);
	HAL_TIM_Base_Stop(&htim2);
	HAL_TIM_Base_Stop(&htim3);
	HAL_TIM_Base_Stop(&htim4);
	HAL_TIM_Base_Stop(&htim5);
	HAL_TIM_Base_Stop(&htim8);
	HAL_TIM_Base_Stop(&htim6);
	HAL_TIM_Base_Stop(&htim7);
	HAL_TIM_Base_Stop(&htim10);
	HAL_TIM_Base_Stop(&htim11);
	HAL_TIM_Base_Stop(&htim12);
	HAL_TIM_Base_Stop(&htim13);
}

void DefaultPatch(void)
{
	/* Load the initial global system settings */
	param 					= 0;			/* Choose no param to edit (default is pitch bend) */
	sus 						= 0;			/* Set the sustain pedal to off */
	transpose 			= -12;		/* Set the transpose to one octave lower */

	/* Load the initial patch parameters */
	paramvalue[0] 	= 64;			/* Set pitch bend to center */
	paramvalue[1] 	= 0;			/* Set the modulation wheel to sine */
	paramvalue[2] 	= 64;			/* Set the master tuning to center */
	paramvalue[3] 	= 0;			/* Set the upper and lower waveforms to sawtooth */
	paramvalue[4] 	= 64;			/* Set oscillator mix to center */
	paramvalue[5] 	= 16;			/* Set the de-tune to 16 */
	paramvalue[6] 	= 64;			/* Set scale to no offset for upper oscillators */
	paramvalue[7] 	= 31;			/* Set resonance to 31 */
	paramvalue[8] 	= 127; 		/* Set the default square wave pw value to 50% */
	paramvalue[9] 	= 0;			/* Set the pwm mod level to off */
	paramvalue[10] 	= 0; 			/* Set VCF attack time */
	paramvalue[11] 	= 30;			/* Set VCF decay time */
	paramvalue[12] 	= 28;			/* Set VCF sustain level */
	paramvalue[13] 	= 8;			/* Set VCF release time */
	paramvalue[14] 	= 0;			/* Set VCA attack time */
	paramvalue[15] 	= 20;			/* Set VCA decay time */
	paramvalue[16] 	= 127;		/* Set VCA sustain level */
	paramvalue[17] 	= 8;			/* Set VCA release time */
	paramvalue[18] 	= 64;			/* Set VCF keyboard follow level to half */
	paramvalue[19] 	= 64;			/* Set ENV keyboard follow level to half */
	paramvalue[20] 	= 48;			/* Set keyboard velocity to drive only VCF */
	paramvalue[21] 	= 127;		/* Set VCF envelope level to full */
	paramvalue[22] 	= 116;		/* Set MOD LFO rate */
	paramvalue[23] 	= 0;			/* Set PWM LFO rate */
	paramvalue[24] 	= 120;		/* Set VCF LFO rate */
	paramvalue[25] 	= 0;			/* Set the vcf mod level to off */
	paramvalue[26] 	= 0;			/* Set the modulation wheel2 to sine */
	paramvalue[27] 	= 127;		/* Set the loundess to 127 */
	
	for(i=0;i<28;i++)
	{
		LocalMidiHandler(i, paramvalue[i]);
	}
}

void SelfTest(void)
{
	seqnote = rand() % 24 + 36;
	chord = rand() % 9;
	switch(chord)
	{
		case 0: // Major
			triad2 = 4;
			triad3 = 7;
			triad4 = 12;
			break;
		case 1: // Minor
			triad2 = 3;
			triad3 = 7;
			triad4 = 12;
			break;
		case 2: // Sus4
			triad2 = 5;
			triad3 = 7;
			triad4 = 12;
			break;
		case 3: // Major7
			triad2 = 4;
			triad3 = 7;
			triad4 = 11;
			break;
		case 4: // Minor7
			triad2 = 3;
			triad3 = 7;
			triad4 = 10;
			break;
		case 5: // Major6
			triad2 = 4;
			triad3 = 9;
			triad4 = 12;
			break;
		case 6: // Minor6
			triad2 = 3;
			triad3 = 9;
			triad4 = 12;
			break;
		case 7: // Dim
			triad2 = 3;
			triad3 = 6;
			triad4 = 12;
			break;
		case 8: // Aug
			triad2 = 4;
			triad3 = 8;
			triad4 = 12;
			break;
	}
	play_note(seqnote,127);
	play_note(seqnote + triad2,127);		
	play_note(seqnote + triad3,127);
	play_note(seqnote + triad4,127);
	HAL_Delay(1600);
	stop_note(seqnote);
	stop_note(seqnote + triad2);
	stop_note(seqnote + triad3);
	stop_note(seqnote + triad4);
	HAL_Delay(800);
}

void SynthOutput(void)
{
	/* Clear the sum of the oscillators */
	summer = 0.0f;

	/* Apply the VCA/VCF with key follow to all voices */
	for(i=0;i<VOICE_NUM;i++)
	{
		channel[i] = channel[i] * (vcacutoff[i] * vcavellvl[i]);
		summer = summer + KarlsenLPF(channel[i], (vcfval * vcfenvlvl) * ((vcfcutoff[i] * vcfkeyfollow[i]) * vcfvellvl[i]), resonance, i);
	}

	/* Get the average of the sum */
	summer = (float)summer / VOICE_NUM;
	
	/* Convert back to a 16-bit integer and apply a bias to center the waveform */
	output = (uint16_t)2048.0f + (2048.0f * summer);
	
	/* Process the delayed signal */
	delaybuffer[delay_in] = output;
	delay_in++;
	if(delay_in == 1000)delay_in = 0;
	delay_out++;
	if(delay_out == 1000)delay_out = 0;
	
	/* Output to DAC directly */
	HAL_DAC_SetValue(&DacHandle, DAC_CHANNEL_1, DAC_ALIGN_12B_R, output);
	HAL_DAC_SetValue(&DacHandle, DAC_CHANNEL_2, DAC_ALIGN_12B_R, delaybuffer[delay_out]);
}

void play_note(uint8_t note, uint8_t velocity)
{
	// If the note is playing already, just re-trigger it
	for(j=0;j<VOICE_NUM;j++)
	{
		if(chanstat[j] == note)
		{
			// Load VCF/VCA ADS parameters.
			if(velsel == 1 | velsel == 3)
				vcfvellvl[j] 		= ((float)(velocity) / 127.0f);
			else
				vcfvellvl[j] 		= 1.0f;
			vcfsustainlvl[j] 	= vcfsustain;
			if(velsel >= 2)
				vcavellvl[j] 		= ((float)(velocity) / 127.0f);
			else
				vcavellvl[j] 		= 1.0f;
			vcasustainlvl[j] 	= vcasustain;
			
			// Set the VCF/VCA ADSR to attack state and cutoff level to zero.
			vcfcutoff[j] 			= 0.0f;
			vcacutoff[j] 			= 0.0f;
			// High
			vcfadsr[j] 				= 1;
			vcaadsr[j] 				= 1;
			return;
		}
	}	
	
	// Cycle thru available voices and assign the note to it, or steal from released voice. 
	for(j=0;j<VOICE_NUM;j++)
	{
		//Check the voice VCF/VCA ADSR if release state
		if(vcfadsr[voice] == STATE_RELEASE | vcaadsr[voice] == STATE_RELEASE | chanstat[voice] == STATE_FREE) // Voice is free or in released state?
		{
			// Load VCF/VCA ADS parameters.
			if(velsel == 1 | velsel == 3)
				vcfvellvl[voice] 		= ((float)(velocity) / 127.0f);
			else
				vcfvellvl[voice] 		= 1.0f;
			vcfsustainlvl[voice] 	= vcfsustain;
			if(velsel >= 2)
				vcavellvl[voice] 		= ((float)(velocity) / 127.0f);
			else 
				vcavellvl[voice] 		= 1.0f;
			vcasustainlvl[voice] 	= vcasustain;
			
			// Set the VCF/VCA ADSR to attack state and cutoff level to zero.
			vcfcutoff[voice] 			= 0.0f;
			vcacutoff[voice] 			= 0.0f;
			vcfadsr[voice] 				= STATE_ATTACK;
			vcaadsr[voice] 				= STATE_ATTACK;
			
			chanstat[voice] 			= note; // Set the channel status to the note number playing
			
			// update the pitch of the voices oscillators.
			switch(voice)
			{
				case(0):
					TIM14_Config(pitchtbl[note + scale]);
					TIM6_Config(pitchtbl[note]);
					break;
				case(1):
					TIM2_Config(pitchtbl[note + scale]);
					TIM7_Config(pitchtbl[note]);
					break;
				case(2):
					TIM3_Config(pitchtbl[note + scale]);
					TIM10_Config(pitchtbl[note]);
					break;
				case(3):
					TIM4_Config(pitchtbl[note + scale]);
					TIM11_Config(pitchtbl[note]);
					break;
				case(4):
					TIM5_Config(pitchtbl[note + scale]);
					TIM12_Config(pitchtbl[note]);
					break;
				case(5):
					TIM8_Config(pitchtbl[note + scale]);
					TIM13_Config(pitchtbl[note]);
					break;
			}
			
			// Move to the next voice and return.
			voice++;
			if(voice > VOICE_NUM-1) voice = 0;
			return;
		}
		
		// Move to the next voice and continue searching.
		voice++;
		if(voice > VOICE_NUM-1) voice = 0;
	}
}

void stop_note(uint8_t note)
{
	/* Find the voice that is playing the note and release it */
	for(n=0;n<VOICE_NUM;n++)
	{
		if(note == chanstat[n]) // Is this the voice that is playing the note?
		{
			vcfadsr[n] = STATE_RELEASE; // Set the voice VCF ADSR to release state
			vcaadsr[n] = STATE_RELEASE; // Set the voice VCA ADSR to release state
		}	
	}
}

void stop_all_note(void)
{
	/* Turn off all notes */
	for(n=0;n<VOICE_NUM;n++)
	{
			vcfadsr[n] = STATE_RELEASE; // Set the voice VCF ADSR to release state
			vcaadsr[n] = STATE_RELEASE; // Set the voice VCA ADSR to release state	
	}
}

void MakeSound(void)
{
	/* Get the timer values (sawtooth oscillators) into an array */
	counter[0] 				= __HAL_TIM_GetCounter(&htim14);
	counter[1] 				= __HAL_TIM_GetCounter(&htim2);
	counter[2] 				= __HAL_TIM_GetCounter(&htim3);
	counter[3] 				= __HAL_TIM_GetCounter(&htim4);
	counter[4] 				= __HAL_TIM_GetCounter(&htim5);
	counter[5] 				= __HAL_TIM_GetCounter(&htim8);
	counter[6] 				= __HAL_TIM_GetCounter(&htim6);
	counter[7] 				= __HAL_TIM_GetCounter(&htim7);
	counter[8] 				= __HAL_TIM_GetCounter(&htim10);
	counter[9] 				= __HAL_TIM_GetCounter(&htim11);
	counter[10] 			= __HAL_TIM_GetCounter(&htim12);
	counter[11] 			= __HAL_TIM_GetCounter(&htim13);
	
	tmp = __HAL_TIM_GetCounter(&htim14);

	/* Calculate the pwm value for the square wave */
	pwmval = pwval << 1;
	
	/* Apply the LFO to the pwm value before generating the square wave */
	pwmval += (pwm * pwmlvl);
	switch (wavesel) {
		case 0:
			for(i=0;i<VOICE_NUM;i++)
			{
				counter[i] = sinVals[(counter[i])%SAMP_NUM];
			} 
			break;
		case 1:
			for(i=0;i<VOICE_NUM;i++)
			{
				counter[i] = sqVals[(counter[i])%SAMP_NUM];
			}
			break;
		case 2:
			for(i=0;i<VOICE_NUM;i++)
			{
				counter[i] = sawVals[(counter[i])%SAMP_NUM];
			}
			break;
		case 3:
			for(i=0;i<VOICE_NUM;i++)
			{
				counter[i] = revSawVals[(counter[i])%SAMP_NUM];
			}
			break;
		case 4:
			for(i=0;i<VOICE_NUM;i++)
			{
				counter[i] = triVals[(counter[i])%SAMP_NUM];
			}
			break;
		case 5:
			for(i=0;i<VOICE_NUM;i++)
			{
				counter[i] = ekgVals[(counter[i])%SAMP_NUM];
			}
			break;
		case 6:
			for(i=0;i<VOICE_NUM;i++)
			{
				counter[i] = sincVals[(counter[i])%SAMP_NUM];
			}
			break;
	}
	switch (wavesel2) {
		case 0:
			for(i=VOICE_NUM;i<VOICE_NUM*2;i++)
			{
				counter[i] = sinVals[(counter[i]+pwval)%SAMP_NUM];
			}
			break;
		case 1:
			for(i=VOICE_NUM;i<VOICE_NUM*2;i++)
			{
				counter[i] = sqVals[(counter[i]+pwval)%SAMP_NUM];
			}
			break;
		case 2:
			for(i=VOICE_NUM;i<VOICE_NUM*2;i++)
			{
				counter[i] = sawVals[(counter[i]+pwval)%SAMP_NUM];
			}
			break;
		case 3:
			for(i=VOICE_NUM;i<VOICE_NUM*2;i++)
			{
				counter[i] = revSawVals[(counter[i]+pwval)%SAMP_NUM];
			}
			break;
		case 4:
			for(i=VOICE_NUM;i<VOICE_NUM*2;i++)
			{
				counter[i] = triVals[(counter[i]+pwval)%SAMP_NUM];
			}
			break;
		case 5:
			for(i=VOICE_NUM;i<VOICE_NUM*2;i++)
			{
				counter[i] = ekgVals[(counter[i]+pwval)%SAMP_NUM];
			}
			break;
		case 6:
			for(i=VOICE_NUM;i<VOICE_NUM*2;i++)
			{
				counter[i] = sincVals[(counter[i]+pwval)%SAMP_NUM];
			}
			break;
	}
				
	/* Convert waveforms to floating point and apply oscillator mix */
	for(i=0;i<VOICE_NUM;i++)
	{
		// Mixing 6 and 6 channel for 2 oscillator
		channel[i] = oscmix * (-1.0f + (((float)(counter[i]*2) / 252.0f)));
		channel[i] += ((1.0f - oscmix) * ((-1.0f + (((float)(counter[i+VOICE_NUM]*2) / 252.0f)))));
		channel[i] *= vco;
	}
}

void UpdateLFOs(void)
{
	/* Update the MOD LFO */
	lfo1cnt--;
	if(lfo1cnt == 0)
	{
		lfo1 = lfo1 + lfo1pol;
		if(lfo1 == 127 | lfo1 == 0) lfo1pol *= -1;
		lfo1cnt = lfo1rate;
	}
	
	modlvl = 1.0f - (((float)(lfo1) / 64.0f));
	
	/* Update the PWM LFO */
	lfo2cnt--;
	if(lfo2cnt == 0)
	{
		lfo2 = lfo2 + lfo2pol;
		if(lfo2 == 127 | lfo2 == 0) lfo2pol *= -1;
		lfo2cnt = lfo2rate;
	}
	
	pwmlvl = 1.0f - (((float)(lfo2)) / 64.0f);
	
	/* Update the VCF LFO */
	lfo3cnt--;
	if(lfo3cnt == 0)
	{
		lfo3 = lfo3 + lfo3pol;
		if(lfo3 == 127 | lfo3 == 0) lfo3pol *= -1;
		lfo3cnt = lfo3rate;
	}
	
	vcflvl = 1.0f - (((float)(lfo3) / 127.0f));
	
}

void ProcessPitch(void)
{
	/* Process the pitch of the notes, start with system tuning value */
	modval = 506;
	
	/* Apply the LFO to modulate all the oscillators */
	modval += (mod * modlvl);
	
	/* Calculate the pitch bend value */
	pbend = 1.0f - (((float)bend / 64.0f));
	
	/* Add in the pitch bend and tuning */
	modval += (int8_t)(pbend * 56.0f) + (63 - tun);
	
	/* Calculate the vcf LFO mod value */
	vcfval = (1.0f - ((float)(vcf) / 127.0f)) + (((float)(vcf) / 127.0f) * vcflvl);
	
	/* Update all of the oscillators */
	TIM14->ARR  =	modval;
	TIM2->ARR  	= modval;
	TIM3->ARR  	= modval;
	TIM4->ARR   =	modval;
	TIM5->ARR  	= modval;
	TIM8->ARR  	= modval;
	/* Add in the de-tune for lower oscillators */
	TIM6->ARR  	= modval + det;
	TIM7->ARR  	= modval + det;
	TIM10->ARR  =	modval + det;
	TIM11->ARR  =	modval + det;
	TIM12->ARR  =	modval + det;
	TIM13->ARR  =	modval + det;
}

void ProcessADSRs(void)
{
	/* Process the VCF/VCA ADSR and apply to all voices */
	for(y=0;y<VOICE_NUM;y++)
	{
		if(chanstat[y] != STATE_FREE) // Is a note currently playing? state 255 is free state
		{
			/* Calculate the key follow values based on the key follow levels */
			vcfkeyfollow[y] = (1 - vcfkflvl) + (((float)(chanstat[y]) / 76.0f) * vcfkflvl);
			envkeyfollow[y] = (1 - envkflvl) + (((float)(chanstat[y]) / 76.0f) * envkflvl);
			
			/* Update the state of the VCF ADSR */
			switch(vcfadsr[y])
			{
				case (STATE_IDLE): // Idle state
					break;
				case (STATE_ATTACK): // Attack state
					// Calculate the VCF attack rate
					vcfattackrate[y] =  1.0f / vcfattack;
					// Ramp up the cutoff by incrementing by VCF attack rate contoured with key follow
					vcfcutoff[y] = vcfcutoff[y] + (vcfattackrate[y] * envkeyfollow[y]);
					if(vcfcutoff[y] >= 1.0f) // Attack level reached yet?
					{
						vcfcutoff[y] = 1.0f;
						vcfadsr[y] = STATE_DECAY; // switch to decay state
					}
					break;
				case (STATE_DECAY): // Decay state
					// Calculate the VCF decay rate
					vcfdecayrate[y] =  (1.0f - vcfsustainlvl[y]) / vcfdecay;
					// Ramp down the cutoff by incrementing by VCF decay rate contoured with key follow
					vcfcutoff[y] = vcfcutoff[y] - (vcfdecayrate[y] * envkeyfollow[y]);
					if(vcfcutoff[y] <= vcfsustainlvl[y]) // Decay level reached yet?
					{
						vcfcutoff[y] = vcfsustainlvl[y] + vcfreleaselvl; // ensure sustainlvl > releaselvl
						vcfadsr[y] = STATE_SUSTAIN; // switch to sustain state
					}
					break;
				case (STATE_SUSTAIN): // Sustain state
					// Just hold the VCF cutoff value steady until note is released
					break;
				case (STATE_RELEASE): // Release state
					// Calculate the VCF release rate
					vcfreleaserate[y] =  vcfcutoff[y] / vcfrelease;
					// Ramp down the cutoff by incrementing by VCF release rate
					vcfcutoff[y] = vcfcutoff[y] - (vcfreleaserate[y] * envkeyfollow[y]);
					if(vcfcutoff[y] <= vcfreleaselvl) // Release level reached yet?
					{
						vcfcutoff[y] = 0.0f;
						vcfadsr[y] = STATE_IDLE; // switch to idle state
						chanstat[y] = STATE_FREE; // Set voice to no note playing
					}
					break;
			}
			
			/* Update the state of the VCA ADSR */
			switch(vcaadsr[y])
			{
				case (STATE_IDLE): // Idle state
					break;
				case (STATE_ATTACK): // Attack state
					// Calculate the VCA attack rate
					vcaattackrate[y] =  1.0f / vcaattack;
					// Ramp up the amplitude by incrementing by VCA attack rate contoured with key follow
					vcacutoff[y] = vcacutoff[y] + (vcaattackrate[y] * envkeyfollow[y]);
					if(vcacutoff[y] >= 1.0f) // Attack level reached yet?
					{
						vcacutoff[y] = 1.0f;
						vcaadsr[y] = STATE_DECAY; // switch to decay state
					}
					break;
				case (STATE_DECAY): // Decay state
					// Calculate the VCA decay rate
					vcadecayrate[y] =  (1.0f - vcasustainlvl[y]) / vcadecay;
					// Ramp down the cutoff by incrementing by VCA decay rate contoured with key follow
					vcacutoff[y] = vcacutoff[y] - (vcadecayrate[y] * envkeyfollow[y]);
					if(vcacutoff[y] <= vcasustainlvl[y]) // Decay level reached yet?
					{
						vcacutoff[y] = vcasustainlvl[y] + vcareleaselvl; // ensure sustainlvl > releaselvl
						vcaadsr[y] = STATE_SUSTAIN; // switch to sustain state
					}
					break;
				case (STATE_SUSTAIN): // Sustain state
					// Just hold the VCA cutoff value steady until note is released
					break;
				case (STATE_RELEASE): // Release state
					// Calculate the VCA release rate
					vcareleaserate[y] =  vcacutoff[y] / vcarelease;
					// Ramp down the cutoff by incrementing by VCA release rate
					vcacutoff[y] = vcacutoff[y] - (vcareleaserate[y] * envkeyfollow[y]);
					if(vcacutoff[y] <= vcareleaselvl) // Release level reached yet?
					{
						vcacutoff[y] = 0.0f;
						vcaadsr[y] = 0; // switch to idle state
						chanstat[y] = STATE_FREE; // Set voice to no note playing
					}
					break;
			}
		}
	}
}

// Karlsen 24dB Filter
// b_f = frequency 0..1
// b_q = resonance 0..5

float KarlsenLPF(float signal, float freq, float res, uint8_t m)
{
	static float b_inSH[6], b_in[6], b_f[6], b_q[6], b_fp[6], pole1[6], pole2[6], pole3[6], pole4[6];
	b_inSH[m] 		= signal;
	b_in[m] 			= signal;
	if(freq > 1.0f)freq = 1.0f;
	if(freq < 0.0f)freq = 0.0f;
	b_f[m] 				= freq;
	b_q[m] 				= res;
	uint8_t b_oversample = 0;
		
	while (b_oversample < 2)
	{	
		//2x oversampling
		float prevfp;
		prevfp 			= b_fp[m];
		
		if (prevfp > 1.0f) {prevfp = 1.0f;}	// Q-limiter

		b_fp[m] 		= (b_fp[m] * 0.418f) + ((b_q[m] * pole4[m]) * 0.582f);	// dynamic feedback
		float intfp;
		intfp 			= (b_fp[m] * 0.36f) + (prevfp * 0.64f);	// feedback phase
		b_in[m] 		=	b_inSH[m] - intfp;	// inverted feedback	

		pole1[m] 		= (b_in[m] * b_f[m]) + (pole1[m] * (1.0f - b_f[m]));	// pole 1
		if (pole1[m] > 1.0f) {pole1[m] = 1.0f;} else if (pole1[m] < -1.0f) {pole1[m] = -1.0f;} // pole 1 clipping
		pole2[m] 		= (pole1[m] * b_f[m]) + (pole2[m] * (1 - b_f[m]));	// pole 2
		pole3[m] 		= (pole2[m] * b_f[m]) + (pole3[m] * (1 - b_f[m]));	// pole 3
		pole4[m] 		= (pole3[m] * b_f[m]) + (pole4[m] * (1 - b_f[m]));	// pole 4

		b_oversample++;
	}
	return pole4[m];
}

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow : 
  *            System Clock source            = PLL (HSI)
  *            SYSCLK(Hz)                     = 216000000
  *            HCLK(Hz)                       = 216000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 4
  *            APB2 Prescaler                 = 4
  *            HSI Frequency(Hz)              = 16000000
  *            PLL_M                          = 8
  *            PLL_N                          = 216
  *            PLL_P                          = 2
  *            PLL_Q                          = 9
  *            VDD(V)                         = 3.3
  *            Main regulator output voltage  = Scale1 mode
  *            Flash Latency(WS)              = 7
  * @param  None
  * @retval None
  */
void SystemClock_Config(void)
{  
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /* Initializes the CPU, AHB and APB busses clocks */
  RCC_OscInitStruct.OscillatorType  				=	RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState  							=	RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue   	=	16; // Variable +/- 50khz steps from 0 to 31
  RCC_OscInitStruct.PLL.PLLState  					=	RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource  				  =	RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM  							=	8;
  RCC_OscInitStruct.PLL.PLLN  							=	216;
  RCC_OscInitStruct.PLL.PLLP  							=	RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ  							=	9;
	
	/* Check if initialization is OK */
  if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    Error_Handler();
  
  /* Activate the OverDrive to reach the 216 Mhz Frequency */
  if(HAL_PWREx_EnableOverDrive() != HAL_OK)
    Error_Handler();
  
  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 
     clocks dividers */
  RCC_ClkInitStruct.ClockType  							= (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource  					=	RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider  					= RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider  				=	RCC_HCLK_DIV4;  
  RCC_ClkInitStruct.APB2CLKDivider  				=	RCC_HCLK_DIV4;
	
  if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7) != HAL_OK)
    Error_Handler();

  PeriphClkInitStruct.PeriphClockSelection  =	RCC_PERIPHCLK_CLK48;
  PeriphClkInitStruct.Clk48ClockSelection  	=	RCC_CLK48SOURCE_PLL;
	
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    Error_Handler();

  /* Configure the Systick interrupt time */
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

  /* Configure the Systick */
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
void Error_Handler(void)
{
  /* Infinite loop on any error */
	HAL_GPIO_WritePin(GPIOB, LD3_Pin, GPIO_PIN_SET);      // switch on LED3 
  while(1)
  {    
  } 
}


void DAC_Config(void)
{
	DacHandle.Instance = 					DACx;
	
  if (HAL_DAC_Init(&DacHandle) != HAL_OK)
    Error_Handler();

  sConfig.DAC_Trigger = 				DAC_TRIGGER_NONE;
  sConfig.DAC_OutputBuffer = 		DAC_OUTPUTBUFFER_ENABLE;

  if (HAL_DAC_ConfigChannel(&DacHandle, &sConfig, DACx_CHANNEL) != HAL_OK)
    Error_Handler();
	
	if (HAL_DAC_ConfigChannel(&DacHandle, &sConfig, DACy_CHANNEL) != HAL_OK)
    Error_Handler();

  if (HAL_DAC_Start(&DacHandle, DACx_CHANNEL) != HAL_OK)
    Error_Handler();
	
	if (HAL_DAC_Start(&DacHandle, DACy_CHANNEL) != HAL_OK)
    Error_Handler();
}

void TIM2_Config(uint16_t period)
{
  htim2.Instance = TIM2;

  htim2.Init.Period            = PERIODE_VOICE;
  htim2.Init.Prescaler         = period;
  htim2.Init.ClockDivision     = 0;
  htim2.Init.CounterMode       = TIM_COUNTERMODE_UP;
  htim2.Init.RepetitionCounter = 0;
	htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  HAL_TIM_Base_Init(&htim2);

  HAL_TIM_Base_Start(&htim2);
}

void TIM3_Config(uint16_t period)
{
  htim3.Instance = TIM3;

  htim3.Init.Period            = PERIODE_VOICE;
  htim3.Init.Prescaler         = period;
  htim3.Init.ClockDivision     = 0;
  htim3.Init.CounterMode       = TIM_COUNTERMODE_UP;
  htim3.Init.RepetitionCounter = 0;
	htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  HAL_TIM_Base_Init(&htim3);

  HAL_TIM_Base_Start(&htim3);
}

void TIM4_Config(uint16_t period)
{
  htim4.Instance = TIM4;

  htim4.Init.Period            = PERIODE_VOICE;
  htim4.Init.Prescaler         = period;
  htim4.Init.ClockDivision     = 0;
  htim4.Init.CounterMode       = TIM_COUNTERMODE_UP;
  htim4.Init.RepetitionCounter = 0;
	htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  HAL_TIM_Base_Init(&htim4);

  HAL_TIM_Base_Start(&htim4);
}

void TIM5_Config(uint16_t period)
{
  htim5.Instance = TIM5;

  htim5.Init.Period            = PERIODE_VOICE;
  htim5.Init.Prescaler         = period;
  htim5.Init.ClockDivision     = 0;
  htim5.Init.CounterMode       = TIM_COUNTERMODE_UP;
  htim5.Init.RepetitionCounter = 0;
	htim5.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  HAL_TIM_Base_Init(&htim5);

  HAL_TIM_Base_Start(&htim5);
}

void TIM8_Config(uint16_t period)
{
  htim8.Instance = TIM8;

  htim8.Init.Period            = PERIODE_VOICE;
  htim8.Init.Prescaler         = period;
  htim8.Init.ClockDivision     = 0;
  htim8.Init.CounterMode       = TIM_COUNTERMODE_UP;
  htim8.Init.RepetitionCounter = 0;
	htim8.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  HAL_TIM_Base_Init(&htim8);

  HAL_TIM_Base_Start(&htim8);
}

void TIM6_Config(uint16_t period)
{
  htim6.Instance = TIM6;

  htim6.Init.Period            = PERIODE_VOICE;
  htim6.Init.Prescaler         = period;
  htim6.Init.ClockDivision     = 0;
  htim6.Init.CounterMode       = TIM_COUNTERMODE_UP;
  htim6.Init.RepetitionCounter = 0;
	htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  HAL_TIM_Base_Init(&htim6);

  HAL_TIM_Base_Start(&htim6);
}

void TIM7_Config(uint16_t period)
{
  htim7.Instance = TIM7;

  htim7.Init.Period            = PERIODE_VOICE;
  htim7.Init.Prescaler         = period;
  htim7.Init.ClockDivision     = 0;
  htim7.Init.CounterMode       = TIM_COUNTERMODE_UP;
  htim7.Init.RepetitionCounter = 0;
	htim7.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  HAL_TIM_Base_Init(&htim7);

  HAL_TIM_Base_Start(&htim7);
}

void TIM10_Config(uint16_t period)
{
  htim10.Instance = TIM10;

  htim10.Init.Period            = PERIODE_VOICE;
  htim10.Init.Prescaler         = period;
  htim10.Init.ClockDivision     = 0;
  htim10.Init.CounterMode       = TIM_COUNTERMODE_UP;
  htim10.Init.RepetitionCounter = 0;
	htim10.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  HAL_TIM_Base_Init(&htim10);

  HAL_TIM_Base_Start(&htim10);
}

void TIM11_Config(uint16_t period)
{
  htim11.Instance = TIM11;

  htim11.Init.Period            = PERIODE_VOICE;
  htim11.Init.Prescaler         = period;
  htim11.Init.ClockDivision     = 0;
  htim11.Init.CounterMode       = TIM_COUNTERMODE_UP;
  htim11.Init.RepetitionCounter = 0;
	htim11.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  HAL_TIM_Base_Init(&htim11);

  HAL_TIM_Base_Start(&htim11);
}

void TIM12_Config(uint16_t period)
{
  htim12.Instance = TIM12;

  htim12.Init.Period            = PERIODE_VOICE;
  htim12.Init.Prescaler         = period;
  htim12.Init.ClockDivision     = 0;
  htim12.Init.CounterMode       = TIM_COUNTERMODE_UP;
  htim12.Init.RepetitionCounter = 0;
	htim12.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  HAL_TIM_Base_Init(&htim12);

  HAL_TIM_Base_Start(&htim12);
}

void TIM13_Config(uint16_t period)
{
  htim13.Instance = TIM13;

  htim13.Init.Period            = PERIODE_VOICE;
  htim13.Init.Prescaler         = period;
  htim13.Init.ClockDivision     = 0;
  htim13.Init.CounterMode       = TIM_COUNTERMODE_UP;
  htim13.Init.RepetitionCounter = 0;
	htim13.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  HAL_TIM_Base_Init(&htim13);

  HAL_TIM_Base_Start(&htim13);
}

void TIM14_Config(uint16_t period)
{
  htim14.Instance = TIM14;

  htim14.Init.Period            = PERIODE_VOICE;
  htim14.Init.Prescaler         = period;
  htim14.Init.ClockDivision     = 0;
  htim14.Init.CounterMode       = TIM_COUNTERMODE_UP;
  htim14.Init.RepetitionCounter = 0;
	htim14.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  HAL_TIM_Base_Init(&htim14);

  HAL_TIM_Base_Start(&htim14);
}

void TIM9_Config(uint16_t period)
{
  htim9.Instance = TIM9;

  htim9.Init.Period            = PERIODE_VOICE;
  htim9.Init.Prescaler         = period;
  htim9.Init.ClockDivision     = 0;
  htim9.Init.CounterMode       = TIM_COUNTERMODE_UP;
  htim9.Init.RepetitionCounter = 0;
	htim9.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  HAL_TIM_Base_Init(&htim9);

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;

  HAL_TIMEx_MasterConfigSynchronization(&htim9, &sMasterConfig);

  HAL_TIM_Base_Start_IT(&htim9);
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	/* Run the Mixer routine */
	mixer();
}

void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct;

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();

  /*Configure GPIO pin : User_Blue_Button_Pin */
  GPIO_InitStruct.Pin = User_Blue_Button_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(User_Blue_Button_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : RMII_MDC_Pin RMII_RXD0_Pin RMII_RXD1_Pin */
  GPIO_InitStruct.Pin = RMII_MDC_Pin|RMII_RXD0_Pin|RMII_RXD1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : RMII_REF_CLK_Pin RMII_MDIO_Pin RMII_CRS_DV_Pin */
  GPIO_InitStruct.Pin = RMII_REF_CLK_Pin|RMII_MDIO_Pin|RMII_CRS_DV_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : RMII_TXD1_Pin */
  GPIO_InitStruct.Pin = RMII_TXD1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
  HAL_GPIO_Init(RMII_TXD1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LD3_Pin LD2_Pin LD1_Pin */
  GPIO_InitStruct.Pin = LD3_Pin|LD2_Pin|LD1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_PowerSwitchOn_Pin */
  GPIO_InitStruct.Pin = USB_PowerSwitchOn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(USB_PowerSwitchOn_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_OverCurrent_Pin */
  GPIO_InitStruct.Pin = USB_OverCurrent_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USB_OverCurrent_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : STLK_RX_Pin STLK_TX_Pin */
  GPIO_InitStruct.Pin = STLK_RX_Pin|STLK_TX_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : RMII_TX_EN_Pin RMII_TXD0_Pin */
  GPIO_InitStruct.Pin = RMII_TX_EN_Pin|RMII_TXD0_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LD3_Pin|LD2_Pin|LD1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(USB_PowerSwitchOn_GPIO_Port, USB_PowerSwitchOn_Pin, GPIO_PIN_RESET);
}

/**
  * @brief  CPU L1-Cache enable.
  * @param  None
  * @retval None
  */
void CPU_CACHE_Enable(void)
{
  /* Enable I-Cache */
  SCB_EnableICache();

  /* Enable D-Cache */
  SCB_EnableDCache();
}

/******************************************************************************/
/*   IRQ HANDLER TREATMENT Functions                                          */
/******************************************************************************/

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}

#endif
