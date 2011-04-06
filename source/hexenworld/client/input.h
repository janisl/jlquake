// input.h -- external (non-keyboard) input devices

void IN_Init (void);

void IN_Commands (void);
// oportunity for devices to stick commands on the script buffer

void IN_Move();
// add additional movement on top of the keyboard move cmd

void IN_ModeChanged (void);
// called whenever screen dimensions change

