
// refresh.h -- public interface to refresh functions

/*
 * $Header: /H2 Mission Pack/RENDER.H 4     3/05/98 7:54p Jmonroe $
 */

void V_RenderScene(void);		// must set cl.refdef first
void R_ViewChanged(vrect_t* pvrect, int lineadj, float aspect);
// called whenever cl.refdef or vid change

void R_ParseParticleEffect(void);
void R_ParseParticleEffect2(void);
void R_ParseParticleEffect3(void);
void R_ParseParticleEffect4(void);

int M_DrawBigCharacter(int x, int y, int num, int numNext);
