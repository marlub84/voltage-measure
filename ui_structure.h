/* 
 *
 *
 */
 
 
 #ifndef _UI_STRUCTURE_
 #define _UI_STRUCTURE_
 
 
#include <gtk/gtk.h>

/* 
 * This macro convert code to 
 * name = GTK_WIDGET(gtk_builder_get_object(gtkBuilder, "name"));
 * 
 */
 
 #define CH_GET_OBJECT(builder, name, type, data)		data->name = type(gtk_builder_get_object(builder, #name))

#define CH_GET_WIDGET(builder, name, data) 		CH_GET_OBJECT(builder, name, GTK_WIDGET, data)



typedef struct _CHData ChData;

struct _CHData{
	GtkWidget *AboutWindow;
	GtkWidget *ButtonOK;
	GtkWidget *MainWindow;
	GtkWidget *Uloz;
	GtkWidget *UlozJako;
	GtkWidget *Konec;
	GtkWidget *About;		//object AboutWindow
	GtkWidget *HodnotaAnalog;
	GtkWidget *HodnotaOffset;
	GtkWidget *SetOffset;
	GtkWidget *SetVzorekT;
	GtkWidget *SetVzorekN;
	GtkWidget *UrovenZapal;
	GtkWidget *UrovenSvar;
	GtkWidget *ButtonOpV;
	GtkWidget *ButtonDAC;
	GtkWidget *ButtonOpOut;
	GtkWidget *ButtonSondaOut;
	GtkWidget *ButtonNormal;
	GtkWidget *ButtonNastav;
	GtkWidget *ButtonAutoOffset;
	GtkWidget *ButtonStop;
	GtkWidget *ButtonSondaV;
	GtkWidget *ButtonRead;
	GtkWidget *Informace;
	GtkWidget *DA_level;
	GtkWidget *Calibrate;
	GtkWidget *Open;
	GtkWidget *OpenWindow;
	//Drawind area
	GtkWidget *DataShow;
	
};

	typedef enum {an_vzorek_sonda, an_vzorek_op, dac_value, 
						op_read, sonda_read, calib, normal_work, auto_offset, 
						setting, readSet, stop} PicCommand; 
						
	struct PicData{
		uint16_t AnalogValueOp;
		uint16_t AnalogValueSonda;
		uint8_t picOffset;			// offset			IN	2
		uint8_t picAnVtime;		    // probe delay		IN	1
		uint8_t picAnVnum;			// analog count		IN	3
		uint16_t picUzapal;			// weld detect		IN	4
		uint16_t picUsvar;			// weld ignition 	IN	5
		uint16_t picCalmState[2];	// clam state 		OUT ( auto_offset)
		GtkWindow *Actual;
		uint16_t prumer;
		//uint16_t *receive[6000];
		PicCommand command;		
		uint8_t picDA;
		uint16_t picSonda;
		bool picFiltr;
	};
	//typedef struct Picdata PicData;
	
						
#endif 	/* _UI_STRUCTURE_ */
