int hxc_printf(unsigned char mode,unsigned short x_pos,unsigned short y_pos,char* chaine,...);
int hxc_printf_box(unsigned char mode,char* chaine,...);
void restore_box(void);

void h_line(unsigned short y_pos,unsigned short val);
void invert_line(unsigned short y_pos);
void clear_line(unsigned short y_pos,unsigned short val);
int init_display(void);
void restore_display(void);

#define VERSIONCODE "1.10a"
#define DATECODE "07/09/2015"

extern unsigned char BGCOLOR;
extern unsigned char FGCOLOR;
extern unsigned char ALTBGCOLOR;
extern unsigned char ALTFGCOLOR;
