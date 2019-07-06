/*
 *	11/26/17 by C.P.: Version 0.8.0 - Universal Version
 */

#include "usbd_midi_if.h"
#include "stm32f7xx_hal.h"
 
static uint16_t MIDI_DataRx(uint8_t *msg, uint16_t length);
static uint16_t MIDI_DataTx(uint8_t *msg, uint16_t length);

extern uint8_t key, velocity, ctrl, data;
extern uint8_t wavesel, wavesel2, voicesel, velsel, pwm, mod, vcf, tun, det, sus, notepos, bend, param;
extern int8_t transpose, scale;
extern uint16_t pwval, lfo1rate, lfo2rate, lfo3rate;
extern uint8_t paramvalue[256];
extern void play_note(uint8_t, uint8_t);
extern void stop_note(uint8_t);
extern float vcfkflvl, envkflvl, oscmix, vco, vcfenvlvl, resonance;
extern float vcfattack, vcfdecay, vcfsustain, vcfrelease;
extern float vcaattack, vcadecay, vcasustain, vcarelease;
 
USBD_MIDI_ItfTypeDef USBD_Interface_fops_FS =
{
  MIDI_DataRx,
  MIDI_DataTx
};
 
static uint16_t MIDI_DataRx(uint8_t *msg, uint16_t length)
{
  uint8_t chan 			= msg[1] & 0xf;
  uint8_t msgtype 	= msg[1] & 0xf0;
  uint8_t b1 				=  msg[2];
  uint8_t b2 				=  msg[3];
  uint16_t b 				= ((b2 & 0x7f) << 7) | (b1 & 0x7f);
 
  switch (msgtype)
	{
		case 0x80:
					key 			= b1;
					velocity 	= b2;
					notepos 	= key - 8 + transpose;
					stop_note(notepos);
          HAL_GPIO_TogglePin(GPIOB,GPIO_PIN_7); //blink LED
          break;
		case 0x90:
					key 			= b1;
					velocity 	= b2;
					notepos 	= key - 8 + transpose;
					if(!velocity) stop_note(notepos);
					else play_note(notepos, velocity);
          HAL_GPIO_TogglePin(GPIOB,GPIO_PIN_7); //blink LED
          break;
		case 0xA0:
          break;
		case 0xB0:
					ctrl 			= b1;
					data 			= b2;
		switch(ctrl)
					{
						case (1): // Modulation Wheel
							mod 	= data;
							break;
						case (2): // Wave Select2
							wavesel2 = data >> 4;
							break;
						case (3): // Wave Loudness
							vco 	= (((float)(data)) * 0.007874f);
							break;
						case (16): // Tuning
							tun 	= data;
							break;
						case (17): // Wave Select
							wavesel = data >> 4;
							break;
						case (18): // OSC Mix
							oscmix = (((float)(data)) * 0.007874f);
							break;
						case (19): // De-Tune 
							det 	= data >> 4;
							break;
						case (33): // Scale
							scale = (data - 64) >> 2;
							break;
						case (34): //  
							resonance = (((float)(data)) * 0.007874f * 4.0f);
							break;
						case (35): // Pulse Width Value
							pwval = data;
							break;
						case (36): // PWM LFO Mod Level
							pwm 	= data;
							break;
						case (37): // VCF Attack
							vcfattack = (((float)(data)) * 10.0f);
							break;
						case (38): // VCF Decay
							vcfdecay = (((float)(data)) * 10.0f);
							break;
						case (39): // VCF Sustain
							vcfsustain = (((float)(data)) * 0.007874f);
							break;
						case (40): // VCF Release
							vcfrelease = (((float)(data)) * 10.0f);
							break;
						case (42): // VCA Attack
							vcaattack = (((float)(data)) * 10.0f);
							break;
						case (43): // VCA Decay
							vcadecay = (((float)(data)) * 10.0f);
							break;
						case (44): // VCA Sustain
							vcasustain = (((float)(data)) * 0.007874f);
							break;
						case (45): // VCA Release
							vcarelease = (((float)(data)) * 10.0f);
							break;
						case (48): // VCF Follow Level
							vcfkflvl = (((float)(data)) * 0.007874f);
							break;
						case (49): // ENV Follow Level
							envkflvl = (((float)(data)) * 0.007874f);
							break;
						case (50): // Velocity Select
							velsel = data >> 5;
							break;
						case (51): // VCF Envelope Level
							vcfenvlvl = (((float)(data)) * 0.007874f);
							break;
						case (12): // Mod LFO rate
							lfo1rate = (128 - data) << 2; 
							break;
						case (13): // Pwm LFO rate
							lfo2rate = (128 - data) << 3;
							break;
						case (14): // VCF LFO Mod Level
							vcf = data;
							break;
						case (15): // Vcf LFO rate
							lfo3rate = (128 - data) << 3;
							break;
						case (64): // Sustain pedal controller
							sus = data;
							break;
					}
          break;
		case 0xC0:
					data = b1;
          break;
		case 0xD0:
          break;
		case 0xE0:
          data = b2;
					bend = data;
					break;
		case 0xF0:
					break;
  }
	return 0;
}
 
void LocalMidiHandler(uint8_t param, uint8_t data)
{
	switch(param)
	{
		case (0):  // Pitch Wheel
			bend = data;
			break;
		case (1): // Modulation Wheel
			if(data >= 64)
			{
				mod = (data - 64) >> 1;
			}
			break;
		case (2): // Tuning
			tun = data;
			break;
		case (3): // Wave Select 1
			wavesel = data >> 4;
			break;
		case (4): // OSC Mix
			oscmix = (((float)(data)) * 0.007874f);
			break;
		case (5): // De-Tune 
			det = data >> 4;
			break;
		case (6): // Scale
			scale = (data - 64) >> 2;
			break;
		case (7): // Resonance
			resonance = (((float)(data)) * 0.007874f * 4.0f);
			break;
		case (8): // Pulse Width Value
			pwval = data;
			break;
		case (9): // PWM Level
			pwm = data;
			break;
		case (10): // VCF Attack
			vcfattack = (((float)(data)) * 10.0f);
			break;
		case (11): // VCF Decay
			vcfdecay = (((float)(data)) * 10.0f);
			break;
		case (12): // VCF Sustain
			vcfsustain = (((float)(data)) * 0.007874f);
			break;
		case (13): // VCF Release
			vcfrelease = (((float)(data)) * 10.0f);
			break;
		case (14): // VCA Attack
			vcaattack = (((float)(data)) * 10.0f);
			break;
		case (15): // VCA Decay
			vcadecay = (((float)(data)) * 10.0f);
			break;
		case (16): // VCA Sustain
			vcasustain = (((float)(data)) * 0.007874f);
			break;
		case (17): // VCA Release
			vcarelease = (((float)(data)) * 10.0f);
			break;
		case (18): // VCF Follow Level
			vcfkflvl = (((float)(data)) * 0.007874f);
			break;
		case (19): // ENV Follow Level
			envkflvl = (((float)(data)) * 0.007874f);
			break;
		case (20): // Velocity Select
			velsel = data >> 5;
			break;
		case (21): // VCF Envelope Level
			vcfenvlvl = (((float)(data)) * 0.007874f);
			break;
		case (22): // Mod LFO rate
			lfo1rate = (128 - data) << 2; 
			break;
		case (23): // Pwm LFO rate
			lfo2rate = (128 - data) << 3;
			break;
		case (24): // Vcf LFO rate
			lfo3rate = (128 - data) << 3;
			break;
		case (25): // VCF LFO Mod level
			vcf = data;
			break;
		case (26): // Wave Select 1
			wavesel2 = data >> 4;
			break;
		case (27): // Wave Loudness
			vco = (((float)(data)) * 0.007874f);
			break;
	}
	paramvalue[param] = data;
}

static uint16_t MIDI_DataTx(uint8_t *msg, uint16_t length)
{
  uint32_t i = 0;
  while (i < length) {
    APP_Rx_Buffer[APP_Rx_ptr_in] = *(msg + i);
    APP_Rx_ptr_in++;
    i++;
    if (APP_Rx_ptr_in == APP_RX_DATA_SIZE) {
      APP_Rx_ptr_in = 0;
    }
  }
  return USBD_OK;
}
