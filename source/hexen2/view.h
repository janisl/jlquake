// view.h

extern	QCvar*		v_gamma;

extern float v_blend[4];

extern QCvar* lcd_x;


void V_Init (void);
void V_RenderView (void);
float V_CalcRoll (vec3_t angles, vec3_t velocity);
void V_UpdatePalette (void);

