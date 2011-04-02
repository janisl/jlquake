void KBD_Init();
void KBD_Update(void);
void KBD_Close(void);
void Do_Key_Event(int key, qboolean down);

typedef struct in_state {
	// Pointers to functions back in client, set by vid_so
	void (*IN_CenterView_fp)(void);
	vec_t *viewangles;
	int *in_strafe_state;
} in_state_t;

