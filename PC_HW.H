void init_pc_fdc(unsigned char id);
unsigned char readsector(unsigned char sectornum,unsigned char* data);
unsigned char writesector(unsigned char sectornum,unsigned char* data);
unsigned char wait_function_key(void);
int jumptotrack(unsigned char t);
void quit(void);

#define L_INDIAN(var) var
