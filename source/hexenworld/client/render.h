
// refresh.h -- public interface to refresh functions

struct entity_t
{
	h2entity_state_t state;
	float syncbase;
};

void V_RenderScene (void);		// must set cl.refdef first
void R_ViewChanged (vrect_t *pvrect, int lineadj, float aspect);
								// called whenever cl.refdef or vid change

void R_ParseParticleEffect (void);

void R_ParseParticleEffect2 (void);
void R_ParseParticleEffect3 (void);
void R_ParseParticleEffect4 (void);
void R_ParseRainEffect(void);
void R_DrawName(vec3_t origin, char *Name, int Red);
int M_DrawBigCharacter (int x, int y, int num, int numNext);
