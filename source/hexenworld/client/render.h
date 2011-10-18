
// refresh.h -- public interface to refresh functions

struct entity_t
{
	vec3_t					origin;
	vec3_t					angles;
	vec3_t					angleAdd;		// For clientside rotation stuff
	qhandle_t				model;			// 0 = no model
	int						frame;
	byte					colorshade;
	int						skinnum;		// for Alias models
	int						scale;			// for Alias models
	int						drawflags;		// for Alias models
	int						abslight;		// for Alias models

	struct player_info_s	*scoreboard;	// identify player

	float					syncbase;
};


extern float RTint[256],GTint[256],BTint[256];

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
