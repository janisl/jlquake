void KBD_Init();
void KBD_Update(void);
void KBD_Close(void);
void Do_Key_Event(int key, qboolean down);

struct in_state_t
{
	// Pointers to functions back in client, set by vid_so
	void (*IN_CenterView_fp)(void);
	vec_t *viewangles;
	int *in_strafe_state;
};

void RW_IN_Init(in_state_t *in_state_p);
void RW_IN_Shutdown(void);
void RW_IN_Move (usercmd_t *cmd);
void RW_IN_Frame (void);
void RW_IN_Activate();
