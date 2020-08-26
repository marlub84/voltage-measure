/*
 * Using UI from glade
 * 
 */

#include <gtk/gtk.h>
#include <gmodule.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
//#include <cairo.h>

#include "ui_structure.h"
#include "myutil.h"

#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define TIME_MEM        5                  // time remamber in minutes                  
#define MEM_SIZE		6000
#define NEXT_POINT		10
#define CORRECT(value)			((value) / 2)


// thread id global
GThread *tid, *tid1;
//uint16_t receiveData;
uint16_t *Optable[(TIME_MEM * MEM_SIZE)];
int RvIndex;
bool RECIV = TRUE;
// serial port stream
int gfd;
bool NewData[6];

cairo_surface_t *Image;

void OpenPort()
{
	struct termios myport;
	// open port
	const char *device = "/dev/ttyS0";
	gfd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);

	//gfd = open(device, O_RDWR | O_NDELAY);
	if (gfd == -1) {
		printf("Failed to open port %s\n", device);
	}
	// end open port
	
	if (!isatty(gfd)){				// unistd - kontoluje jestli fd ukazuje na terminal
		// error handle
		printf("Contol fd error");
	}
	
	// get the current configuration of the serial port
	
	if (tcgetattr(gfd, &myport) < 0) {
		// error handle
		printf("Get attribute error");
	}
	// configure flags
	myport.c_iflag &= ~(IGNBRK | BRKINT | ICRNL | INLCR | PARMRK | INPCK |
						ISTRIP | IXON);
	myport.c_oflag &= 0;//OPOST;
	
	myport.c_lflag &= ~(ISIG | ICANON | ECHO | IEXTEN | ECHONL);
	
	myport.c_cflag &= ~(CSIZE | PARENB);
	
	myport.c_cflag |= CS8;
	
	myport.c_cc[VMIN] = 1;
	myport.c_cc[VTIME] = 0;

	// communication speed
	if (cfsetispeed(&myport, B115200) < 0 || cfsetospeed(&myport, B115200) < 0){
		// error handle
		printf("Speed set error");
	}
	// apply the configuration
	if (tcsetattr(gfd, TCSAFLUSH, &myport) < 0) {
		// error handle
		printf("Attribute set error");
	}
	printf("Open port End\n");
}

gpointer SerialReceive(gpointer widget);

void SaveFile()
{
	FILE *pfile;
	char nameFile[] = "data-";
	char extFile[] = ".dat";
	char filename[40];
	char tmp_data[10];
	char tmp_time[10];
	TimeToChar(&tmp_time);
	DateToChar(&tmp_data);
	strcpy(filename, nameFile);
	strcat(filename, tmp_data);
	strcat(filename, tmp_time);
	strcat(filename, extFile);
	g_print ("Filename : %s\n", filename);
	uint16_t table[(TIME_MEM * MEM_SIZE)];
	int i;
	for (i = 0; i < (TIME_MEM * MEM_SIZE); i++)
	{
		table[i] = *(Optable[i]);
	}
	pfile = fopen(filename, "w");
	fwrite(table, 2, (TIME_MEM * MEM_SIZE) + 1, pfile);
	
	fclose(pfile);
}

void Serial_Send(gpointer dSend)
{
    struct PicData *myData = (struct PicData *) dSend;
    PicCommand com;
    com = ((PicCommand ) myData->command);
    RECIV = TRUE;
    uint8_t sendL, sendH;
	uint16_t num;
	char arg;
	int rv;
	int l;
			
    switch(com)
    {
		case an_vzorek_sonda:
			g_print("an_vzorek_sonda \n");
			
		 	num = 1;						// nastav na sondu
			arg = num;						// konvertuj na 1 bajty (char)
			//prumerS = 0;
			//minS = 0;
			//maxS = 0;
			
			write(gfd, &arg, 1);				// posli data do mcu
			tid = g_thread_new("Receive", &SerialReceive, myData);
			
			
			break;
		case an_vzorek_op:
			g_print("an_vzorek_op \n");
			num = 2;						// nastav na op
			arg = num;						// konvertuj na 1 bajty (char)

			write(gfd, &arg, 1);				// posli data do mcu
			tid = g_thread_new("Receive", &SerialReceive, myData);
			
			break;
		case dac_value:
		// NOT USE 
			g_print("dac_value\n");
			sendH = myData->picDA;
			
			num = 3;
			arg = num;
			write(gfd, &arg, 1);				// posli data do mcu
			arg = sendH;
			write(gfd, &arg, 1);
			//tid = g_thread_new("Receive", &SerialReceive, myData);
			
			break;
        case op_read:
			g_print("op_read\n");
			num = 4;
			arg = num;
			write(gfd, &arg, 1);				// posli data do mcu
			tid1 = g_thread_new("Receive", &SerialReceive, myData);
			
        break;
        case sonda_read:
			g_print("sonad_read\n");
			num = 5;
			arg = num;
			write(gfd, &arg, 1);				// posli data do mcu
			tid1 = g_thread_new("Receive", &SerialReceive, myData);
			
        break;
        case calib:
			g_print("Calibrate \n");
			num = 8;
			arg = num;
			write(gfd, &arg, 1);				// posli data do mcu
		break;	
        case normal_work:
					
			printf("diagnostic");
				arg = 0x01;
				write(gfd, &arg, 1);
				arg = 0x02;
				write(gfd, &arg, 1);
				arg = 0x00;
				write(gfd, &arg, 1);
				arg = 0x00;
				write(gfd, &arg, 1);
				arg = 0x00;
				write(gfd, &arg, 1);
				arg = 0x08;
				write(gfd, &arg, 1);
				arg = 0x79;
				write(gfd, &arg, 1);
				arg = 0xCC;
				write(gfd, &arg, 1);
				printf(" Send all");
				rv = 0;
				l = 0;
				unsigned char op[10];
				
				while(rv < 10)
				{
					rv = read(gfd,&op[l],1);
					if (rv == 1){
						printf("Receive data : %i \n", op[l]);
						rv = 0;
						l++;
					}
				}
			//g_print("normal_work\n");
			//num = 6;
			//arg = num;
			////write(gfd, &arg, 1);				// posli data do mcu
			//tid = g_thread_new("Receive", &SerialReceive, myData);
			
        break;
        case auto_offset:
			// clam state
			g_print("auto_offset\n");
			num = 7;
			arg = num;
			write(gfd, &arg, 1);				// posli data do mcu
			tid = g_thread_new("Receive", &SerialReceive, myData);
        break;
        case setting:
			g_print("Write setting\n");
			num = 9;
			arg = num;
			write(gfd, &arg, 1);				// posli data do mcu
			
			// probe delay			IN	1			
			write(gfd, &myData->picAnVtime, 1);
			g_print("Write setting probe %i\n", myData->picAnVtime);
			// offset				IN	2			
			write(gfd, &myData->picOffset, 1);
			g_print("Write setting offset %i\n", myData->picOffset);
			// analog count			IN	3			
			write(gfd, &myData->picAnVnum, 1);
			g_print("Write setting count %i\n", myData->picAnVnum);
			// weld detect			IN	4			
			sendL = myData->picUsvar;
			sendH = (myData->picUsvar) >> 8;
			write(gfd, &sendL, 1);
			write(gfd, &sendH, 1);
		
			g_print("Write setting weld %i\n", myData->picUsvar);

			if (myData->picFiltr == true){
				sendL = "Y";
				write(gfd, &sendL, 1);
				g_print("Write setting filtr : %i\n", myData->picFiltr);
			}
			else {
				sendL = "N";
				write(gfd, &sendL, 1);
				g_print("Write setting filtr : %i\n", myData->picFiltr);
			}
			// D/A value
			//write(gfd, &myData->picDA, 1);
			//g_print("Write setting D/A %i\n", myData->picDA);
        break;
        case readSet:
			g_print("Read setting\n");
			num = 10;
			arg = num;
			write(gfd, &arg, 1);
			tid = g_thread_new("Receive", &SerialReceive, myData);
			break;
        case stop:
			g_print("stop\n");
			num = 15;
			arg = num;
			write(gfd, &arg, 1);				// posli data do mcu
        break;            
    }
}

gpointer SerialReceive(gpointer myData)
{
	struct PicData *wid = (struct PicData *) myData;
    g_print("Serial - in use\n");
	PicCommand com = (PicCommand) wid->command;
	uint16_t receiveData, rcvL, rcvH;
	
	uint16_t begin;
	uint16_t count;
	uint16_t rcv = 0;
	int num;
	char arg;
	if (com == readSet){
		// probe delay			IN	1 (1 byte)
		while (RECIV){
			rcv = read(gfd, &receiveData, 1);
			if (rcv == 1){
				wid->picAnVtime = receiveData; 
				g_print("Read %i\n", receiveData);
				RECIV = FALSE;
			}
			
		}
		RECIV = true;
		rcv = 0;
		// offset				IN	2 (1 byte)
		while (RECIV){
			rcv = read(gfd, &receiveData, 1);
			if (rcv == 1){
				wid->picOffset = receiveData;
				g_print("Read %i\n", receiveData); 
				RECIV = FALSE;
			}
		}
		RECIV = true;
		rcv = 0;	
		// analog count			IN	3 (1 byte)
		while (RECIV){
			rcv = read(gfd, &receiveData, 1);
			if (rcv == 1){
				wid->picAnVnum = receiveData; 
				g_print("Read %i\n", receiveData);
				RECIV = FALSE;
			}
		}
		RECIV = true;
		rcv = 0;	
		// weld detect			IN	4 (2 byte)
		while (RECIV){
			rcv = read(gfd, &receiveData, 2);
			if (rcv == 2){
				wid->picUsvar = receiveData; 
				g_print("Read %i\n", receiveData);
				RECIV = FALSE;
			}
		}
		RECIV = true;
		rcv = 0;			
		// D/A value	 		IN	6 (1 byte)
		
		while (RECIV){
			rcv = read(gfd, &receiveData, 1);
			if (rcv == 1){
				wid->picDA = receiveData; 
				g_print("Read %i\n", receiveData);
				RECIV = FALSE;
			}
		}
		RECIV = true;
		rcv = 0;			
		// filtr On/Off	 		IN	6 (1 byte)
		
		while (RECIV){
			rcv = read(gfd, &receiveData, 1);
			if (rcv == 1){
				if (receiveData == 'Y'){
					wid->picFiltr = TRUE;
				}else wid->picFiltr = FALSE;
				
				RECIV = FALSE;
			}
		}
		int i;
		for (i = 0; i < 6; i++){
			NewData[i] = TRUE;
		}
	}else if( com == auto_offset){
		RECIV = TRUE;
		// receive 2 value 16 bit one
		while (RECIV){
			rcv = read(gfd, &receiveData, 1);
			if (rcv == 1){
				rcvL = receiveData;
				RECIV = TRUE;
				rcv = 0;
				while(RECIV){
					rcv = read(gfd, &receiveData, 1);
					if (rcv == 1){
						rcvH = receiveData;
						RECIV = FALSE;
					}
				}
				wid->picCalmState[0] = rcvL + (rcvH << 8); 
				g_print("Read %i\n", wid->picCalmState[0]);
				
			}
		}
		RECIV = TRUE;
		rcv = 0;
		while (RECIV){
			rcv = read(gfd, &receiveData, 2);
			if (rcv == 2){
				wid->picCalmState[1] = receiveData; 
				g_print("Read %i\n", receiveData);
				RECIV = FALSE;
			}
		}
		
	}else if ((com == an_vzorek_sonda) || (com == an_vzorek_op)){
		count = (uint16_t) wid->picAnVnum;
		begin = 0;
		rcv = 0;
		while(begin <= count)
		{
			//Test
			//*(Optable[begin]) = (uint16_t) 500;
			//End test
			rcv = read(gfd, &receiveData, 2);
			if (rcv == 2){
				*(Optable[begin]) = receiveData;
				g_print("Mam data %i s %i\n", receiveData, begin);
				begin += 1;
				rcv = 0;
			}
			
		}
	}
	else if (com == normal_work){
		
		RvIndex = 0;
		while(RECIV)
		{
			
			rcv = read(gfd, &receiveData, 2);
			if (rcv == 2){
				*(Optable[RvIndex]) = receiveData;
				g_print("Mam data %i s %i\n", receiveData, RvIndex);
				rcv = 0;
				RvIndex += 1;
			}
			if (RvIndex == (MEM_SIZE * TIME_MEM))
			{
				//g_print("Receive 2 - new cycle");
				RvIndex = 0;
				//RECIV = FALSE;
				//SaveFile();
				g_print("Receive 2 - finish");
			}
				/*
				if (receiveData == 10){
					bool indata = TRUE;
					while(indata){
						rcv = 0;
						rcv = read(gfd, &receiveData, 2);
						if (rcv == 2){
							wid->picClamState[0] = receiveData; 
							indata = FALSE;
						}
					}
					rcv = 0;
					indata = TRUE;
					while(indata){
						rcv = 0;
						rcv = read(gfd, &receiveData, 2);
						if (rcv == 2){
							wid->picClamState[0] = receiveData; 
							indata = FALSE;
						}
					}
				}else {
					*(Optable[begin]) = receiveData;
					g_print("Mam data %i s %i\n", receiveData, begin);
					begin += 1;
					rcv = 0;
				}
				if (begin >= (MEM_SIZE * TIME_MEM)){
					begin = 0;
					g_print("Receive normal - start new");
				}
			}*/
			
		}
			g_print("stop1\n");
			num = 15;
			arg = num;
			write(gfd, &arg, 1);
		
	}
	else {
		count = MEM_SIZE;
		RvIndex = 0;
		while (RECIV)
		{
			if (!(RECIV)) break;
			rcv = read(gfd, &receiveData, 2);
			
			if (rcv == 2){
				*(Optable[RvIndex]) = receiveData;
				g_print("Mam data %i s %i\n", receiveData, RvIndex);
				rcv = 0;
				RvIndex += 1;
			}
			if (RvIndex == (MEM_SIZE * TIME_MEM))
			{
				//g_print("Receive 2 - new cycle");
				RvIndex = 0;
				//RECIV = FALSE;
				//SaveFile();
				g_print("Receive 2 - finish");
			}
			
		}
			g_print("stop1\n");
			num = 15;
			arg = num;
			write(gfd, &arg, 1);
	}
	
	//g_signal_emit_by_name(wid, "dataReceive");
	return 0;
}

void dataReset ()
{
	// reset table
	int i;
    for (i = 0 ; i < (TIME_MEM * MEM_SIZE); i++)
    {
		*(Optable[i]) = (uint16_t) 0;
    }
    //myData->AnalogValueOp = 0;
	g_print("Data reset\n");
	
}

// Value show
G_MODULE_EXPORT void on_HodnotaOffset_activated(GtkWidget *widget, cairo_t *cr, gpointer data)
{
	struct PicData *mydata = (struct PicData *) data;
	uint16_t tmp = (uint16_t) mydata->picCalmState[0];
	uint16_t tmp1 = (uint16_t) mydata->picCalmState[1];
	char itoc[14];
    sprintf(itoc, "%i - %i", tmp, tmp1);
    gtk_entry_set_text(GTK_ENTRY(widget), itoc);

}

G_MODULE_EXPORT void on_HodnotaAnalog_activated(GtkWidget *widget, cairo_t *cr, gpointer data)
{
	struct PicData *mydata = (struct PicData *) data;
	uint16_t tmp = (uint16_t) mydata->AnalogValueOp;
	char itoc[10];
    sprintf(itoc, "%i", (tmp));
    gtk_entry_set_text(GTK_ENTRY(widget), itoc);
}

// button handler

G_MODULE_EXPORT void on_ButtonOpV_clicked (GtkWidget *widget, gpointer data)
{
	
	struct PicData *tmp = (struct PicData*) data;
	PicCommand c = an_vzorek_op;
	tmp->command = c;
	//tmp->Actual = widget;
	dataReset ();
	Serial_Send(tmp);
	
}

G_MODULE_EXPORT void on_ButtonSondaV_clicked (GtkWidget *widget, gpointer data)
{
    /* posli data na serial port 
    - poslat signal ze data poslane a ocekavame data s mcu
    */
    struct PicData *tmp = (struct PicData*) data;
    PicCommand c = an_vzorek_sonda; 
    tmp->command = c;
	dataReset ();
	Serial_Send(tmp);   
}

G_MODULE_EXPORT void on_ButtonDAC_clicked (GtkWidget *widget, gpointer data)
{
	struct PicData *tmp = (struct PicData*) data;
    PicCommand c = dac_value;
	tmp->command = c;
	Serial_Send(tmp);
	
}

G_MODULE_EXPORT void on_ButtonOpOut_clicked (GtkWidget *widget, gpointer data)
{
	struct PicData *tmp = (struct PicData*) data;
    PicCommand c = op_read;
	tmp->command = c;
	dataReset ();
	Serial_Send(tmp);	
}

G_MODULE_EXPORT void on_ButtonSondaOut_clicked (GtkWidget *widget, gpointer data)
{
	struct PicData *tmp = (struct PicData*) data;
    PicCommand c = sonda_read;
	tmp->command = c;
	dataReset ();
	Serial_Send(tmp);
}

G_MODULE_EXPORT void on_ButtonNormal_clicked (GtkWidget *widget, gpointer data)
{
	struct PicData *tmp = (struct PicData*) data;
    PicCommand c = normal_work;
	tmp->command = c;
	dataReset ();
	Serial_Send(tmp);
}

G_MODULE_EXPORT void on_ButtonNastav_clicked (GtkWidget *widget, gpointer data)
{
	struct PicData *tmp = (struct PicData*) data;
    PicCommand c = setting;
	tmp->command = c;
	Serial_Send(tmp);
}

G_MODULE_EXPORT void on_ButtonAutoOffset_clicked (GtkWidget *widget, gpointer data)
{
	struct PicData *tmp = (struct PicData*) data;
    PicCommand c = auto_offset;
	tmp->command = c;
	Serial_Send(tmp);
}

G_MODULE_EXPORT void on_ButtonStop_clicked (GtkWidget *widget, gpointer data)
{
	struct PicData *tmp = (struct PicData*) data;
    PicCommand c = stop;
	tmp->command = c;
	Serial_Send(tmp);
	RECIV = FALSE;
}

G_MODULE_EXPORT void on_ButtonRead_clicked (GtkWidget *widget, gpointer data)
{
	struct PicData *tmp = (struct PicData*) data;
    PicCommand c = readSet;
	tmp->command = c;
	Serial_Send(tmp);
}

G_MODULE_EXPORT void on_Calibrate_clicked (GtkWidget *widget, gpointer data)
{
	struct PicData *tmp = (struct PicData*) data;
    PicCommand c = calib;
	tmp->command = c;
	Serial_Send(tmp);
}

///// MENUBAR

G_MODULE_EXPORT void on_About_activate(GtkWidget *widget, gpointer data){
	
	gtk_widget_show(data);
	
}

G_MODULE_EXPORT void on_Uloz_activate (GtkWidget *widget, gpointer data)
{
	FILE *pfile;
	char nameFile[] = "data-";
	char extFile[] = ".dat";
	char filename[40];
	char tmp_data[10];
	char tmp_time[10];
	TimeToChar(&tmp_time);
	DateToChar(&tmp_data);
	strcpy(filename, nameFile);
	strcat(filename, tmp_data);
	strcat(filename, tmp_time);
	strcat(filename, extFile);
	g_print ("Filename : %s\n", filename);
	uint16_t table[(TIME_MEM * MEM_SIZE)];
	int i;
	for (i = 0; i < (TIME_MEM * MEM_SIZE); i++)
	{
		table[i] = *(Optable[i]);
	}
	pfile = fopen(filename, "w");
	fwrite(table, 2, (TIME_MEM * MEM_SIZE) + 1, pfile);
	
	fclose(pfile);
	
}

G_MODULE_EXPORT void on_Open_activate (GtkWidget *widget, gpointer data)
{
	GtkWidget *dialog;
	
	GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;	
	gint res;
	dialog = gtk_file_chooser_dialog_new ("Open File",
                                      NULL,
                                      action,
                                      ("_Cancel"),
                                      GTK_RESPONSE_CANCEL,
                                      ("_Open"),
                                      GTK_RESPONSE_ACCEPT,
                                      NULL);
	
	GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
	res = gtk_dialog_run (GTK_DIALOG (dialog));
	if (res == GTK_RESPONSE_ACCEPT)
	  {
	    char *filename;
	    
	    filename = gtk_file_chooser_get_filename (chooser);
	    FILE *pfile;
	    pfile = fopen(filename, "r");
	    uint16_t table[(TIME_MEM * MEM_SIZE)];
		fread(table, 2, (TIME_MEM * MEM_SIZE), pfile);
		
		int i;
		for (i = 0; i < (TIME_MEM * MEM_SIZE); i++)
		{
			*(Optable[i]) = table[i];
		}
	    fclose(pfile);
	    
	  }
	
	gtk_widget_destroy (dialog);
}


// Mcu data

G_MODULE_EXPORT void on_SetVzorekT_value_changed (GtkWidget *widget, gpointer data)
{
    struct PicData *tmp = (struct PicData *) data;
    if (NewData[0]){
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), tmp->picAnVtime);
		NewData[0] = FALSE;
	}
    tmp->picAnVtime = (uint8_t) gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
       	
}

G_MODULE_EXPORT void on_SetVzorekN_value_changed (GtkWidget *widget, gpointer data)
{
    struct PicData *tmp = (struct PicData *) data;
    if (NewData[1]){
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), tmp->picAnVnum);
		NewData[1] = FALSE;
	}
    tmp->picAnVnum = (uint8_t) gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
        	
}

G_MODULE_EXPORT void on_UrovenZapal_value_changed (GtkWidget *widget, gpointer data)
{
    struct PicData *tmp = (struct PicData *) data;
    if (NewData[2]){
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), tmp->picUzapal);
		NewData[2] = FALSE;
	}
    tmp->picUzapal = (uint16_t) gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
        
}

G_MODULE_EXPORT void on_UrovenSvar_value_changed (GtkWidget *widget, gpointer data)
{
	struct PicData *tmp = (struct PicData *) data;
	if (NewData[3]){
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), tmp->picUsvar);
		NewData[3] = FALSE;
	}
    tmp->picUsvar = (uint16_t) gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
        
}

G_MODULE_EXPORT void on_SetOffset_value_changed (GtkWidget *widget, gpointer data)
{
    struct PicData *tmp = (struct PicData *) data;
    if (NewData[4]){
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), tmp->picOffset);
		NewData[4] = FALSE;
	}
    tmp->picOffset = (uint8_t) gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
	
}

G_MODULE_EXPORT void on_DA_level_value_changed (GtkWidget *widget, gpointer data)
{
	struct PicData *tmp = (struct PicData *) data;
    if (NewData[5]){
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), tmp->picDA);
		NewData[5] = FALSE;
	}
    tmp->picDA = (uint8_t) gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
	
}


//set value
/*
G_MODULE_EXPORT void on_SetVzorekT_draw (GtkWidget *widget, gpointer newdata, gpointer data)
{
    struct PicData *tmp = (struct PicData *) data;
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), tmp->picAnVtime);
}

G_MODULE_EXPORT void on_SetVzorekN_draw (GtkWidget *widget, gpointer newdata, gpointer data)
{
    struct PicData *tmp = (struct PicData *) data;
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), tmp->picAnVnum);
}

G_MODULE_EXPORT void on_UrovenZapal_draw (GtkWidget *widget, gpointer newdata,  gpointer data)
{
    struct PicData *tmp = (struct PicData *) data;
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), tmp->picUzapal);
}
    
G_MODULE_EXPORT void on_UrovenSvar_draw (GtkWidget *widget, gpointer newdata, gpointer data)
{
	struct PicData *tmp = (struct PicData *) data;
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), tmp->picUsvar);    
}

G_MODULE_EXPORT void on_SetOffset_draw (GtkWidget *widget, gpointer newdata, gpointer data)
{
    struct PicData *tmp = (struct PicData *) data;
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), tmp->picOffset);
}
*/


G_MODULE_EXPORT void on_DataShow_drag_motion (GtkWidget *widget, gpointer data)
{
	g_signal_emit_by_name(widget, "draw");
}

G_MODULE_EXPORT gboolean on_DataShow_event (GtkWidget *widget, gpointer data)
{
	//g_print("Event\n");
	return TRUE;
}

G_MODULE_EXPORT gboolean on_DataShow_expose_event (GtkWidget *widget, gpointer data)
{
	g_print("Event\n");
	return TRUE;
}

G_MODULE_EXPORT void on_DataShow_draw (GtkWidget *widget, cairo_t *cr, gpointer data)
{
	//g_print("Draw\n");
	struct PicData *mydata = (struct PicData *) data;
	
	guint width, height;
	GdkRGBA color;
	GtkStyleContext *context;
	int osX, osY, osYy;
	int i;
	
	uint16_t Usvar, Uzapal, offsetL, offsetH;
	Usvar = CORRECT((mydata->picUsvar));
	Uzapal = CORRECT((mydata->picUzapal));
	
	offsetL = CORRECT((mydata->picCalmState[0]));
	offsetH = CORRECT((mydata->picCalmState[1]));

	/*
	color.red = 0.5;
	color.green = 0.0;
	color.blue = 0.0;
	color.alpha = 0.3;
	*/
	context = gtk_widget_get_style_context (widget);
	
	width = gtk_widget_get_allocated_width (widget);
	height = gtk_widget_get_allocated_height (widget);
	
	gtk_render_background (context, cr, 0, 0, width, height);
	/*
	// line level for picUsvar low
	cairo_new_path (cr);
	cairo_fill (cr);
	cairo_set_source_rgb (cr, 0, 0, 1);		// set color
	cairo_set_line_width (cr, 1.0);	
	
	cairo_move_to (cr, 0, (offsetL - Usvar));
	cairo_line_to (cr, width, (offsetL - Usvar));
	cairo_stroke(cr);
	// line level for picUzapal low
	cairo_new_path (cr);
	cairo_set_source_rgb (cr, 0, 1, 1);		// set color
	cairo_move_to (cr, 0, (offsetL - Uzapal));
	cairo_line_to (cr, width, (offsetL - Uzapal));
	cairo_stroke(cr);
	
	// line level for picUsvar high
	cairo_new_path (cr);
	cairo_fill (cr);
	cairo_set_source_rgb (cr, 0, 0, 1);		// set color
	cairo_set_line_width (cr, 1.0);	
	
	cairo_move_to (cr, 0, (offsetH + Usvar));
	cairo_line_to (cr, width, (offsetH + Usvar));
	cairo_stroke(cr);
	// line level for picUzapal high
	cairo_new_path (cr);
	cairo_set_source_rgb (cr, 0, 1, 1);		// set color
	cairo_move_to (cr, 0, (offsetH + Uzapal));
	cairo_line_to (cr, width, (offsetH + Uzapal));
	cairo_stroke(cr);
	*/
	
	// line level for picOffsetL
	cairo_new_path (cr);
	cairo_fill (cr);
	cairo_set_source_rgb (cr, 0.5, 0, 1);		// set color
	cairo_set_line_width (cr, 1.0);	
	
	cairo_move_to (cr, 0, offsetL);
	cairo_line_to (cr, width, offsetL);
	cairo_stroke(cr);
	// line level for picOffsetH
	
	cairo_new_path (cr);
	cairo_move_to (cr, 0, offsetH);
	cairo_line_to (cr, width, offsetH);
	cairo_stroke(cr);
	
    // draw buttom scale
    osX = NEXT_POINT;
    //osY = height - 10;	
    
    cairo_new_path (cr);
	cairo_fill (cr);
	cairo_set_source_rgb (cr, 0, 1, 0);		// set color
	cairo_set_line_width (cr, 1.0);
	
    for (i = 0; i < (MEM_SIZE * TIME_MEM); i++)
    {
		
		osY = height - 10;
		cairo_move_to (cr, osX, osY);
		osY = 5;//osY - 100;
		cairo_line_to (cr, osX, osY);
		osX += NEXT_POINT;
	}
	
	cairo_stroke(cr);
	
	// TEXT
	cairo_select_font_face (cr, "Sans", CAIRO_FONT_SLANT_NORMAL, 
								CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_font_size (cr, 10);
	
	osX = 1000;
	osY = height - 20;
	for (i = 0; i < (MEM_SIZE * TIME_MEM); i += 1000)
	{
		//TODO 
		cairo_move_to (cr, osX, osY);
		cairo_show_text(cr, "1s");
		osX += i;
	}
	cairo_stroke(cr);
	
	// end text
	
    osX = 0;
    osYy = CORRECT((*(Optable[0])));
    // set coursor to begining
    cairo_new_path (cr);
	cairo_fill (cr);
	
    cairo_move_to (cr, osX, osYy);
    osX = NEXT_POINT;
    
    cairo_set_source_rgb (cr, 1, 0, 0);		// set color
    cairo_set_line_width (cr, 1.0);	
    
    for (i = 1; i < (MEM_SIZE * TIME_MEM); i++)
    {
		osY = (uint16_t) CORRECT(*(Optable[i]));
		if (osY == 0) break;
		cairo_line_to (cr, osX, osY);
		osX += NEXT_POINT;
	}
	
	cairo_stroke(cr);
	
	Image = cairo_image_surface_create (2, width, height);
	/*gtk_style_context_get_color (context, gtk_style_context_get_state (context), 
								&color);*/
	gdk_cairo_set_source_rgba (cr, &color);
	
}

G_MODULE_EXPORT void on_Informace_activate(GtkWidget *widget, gpointer data)
{
	struct PicData *mydata = (struct PicData *) data;
	PicCommand com;
	com = ((PicCommand ) mydata->command);
	int a, tmp;
	uint16_t val;
	int sum = 0;
	
	for (a = 0; a < (TIME_MEM * MEM_SIZE) ; a++)
	{
		val = (uint16_t) *(Optable[a]);
		if (val == 0) break;
		sum += *(Optable[a]);
	}
	
	if ((sum != 0) || (a != 0))
			tmp = sum / a;
	
	if (com == op_read || com == sonda_read)
	{
		for (a = 0; a < RvIndex; a++)
		{
			//val = *(Optable[a]);
			sum += *(Optable[a]);
		}
		if ((sum != 0) || (a != 0))
			tmp = sum / a;
	}
	(mydata->prumer) = tmp;
	g_print("Prumer %i\n", tmp);
}


int main (int argc, char *argv[])
{
	GError *err = NULL;
	ChData *dataSte;
	GtkBuilder *gtkBuilder;
	// User variable
	//PicData pData;
	struct PicData *picdata;
	
    int i;
    //uint16_t *Optable[(TIME_MEM * MEM_SIZE)];
    
    //picdata->table = TIME_MEM;
    
	// Alocate data structure
	dataSte = g_slice_new(ChData);
	picdata = g_slice_new(struct PicData);
	
    for (i = 0 ; i < (TIME_MEM * MEM_SIZE); i++)
    {
		Optable[i] = (uint16_t*) g_slice_alloc(sizeof(uint16_t));
        
    }
    
    // write null to table
    for (i = 0 ; i < (TIME_MEM * MEM_SIZE); i++)
    {
		*(Optable[i]) = (uint16_t) 0;
        
    }
	
	time_t mytime;
	mytime = time(NULL);
	g_print ("Start time of the program : %s\n", ctime(&mytime));
		
	gtk_init(&argc, &argv);
	
	gtkBuilder = gtk_builder_new();
	if ( 0 == gtk_builder_add_from_file(gtkBuilder, "analog.glade", &err)){
			fprintf(stderr, "Chyba pridani s souboru. Error: %s\n", err->message);
	}

	// Get objects from UI
#define GW(name)		CH_GET_WIDGET(gtkBuilder, name, dataSte)
	GW(AboutWindow);
	//GW(ButtonOK);
	GW(MainWindow);
	GW(Uloz);
	GW(UlozJako);
	GW(Konec);
	GW(About);
	GW(HodnotaAnalog);
	GW(HodnotaOffset);
	GW(SetOffset);
	GW(SetVzorekN);
	GW(SetVzorekT);
	GW(UrovenZapal);
	GW(UrovenSvar);
	GW(ButtonOpV);
	GW(ButtonDAC);
	GW(ButtonOpOut);
	GW(ButtonSondaOut);
	GW(ButtonNormal);
	GW(ButtonNastav);
	GW(ButtonAutoOffset);
	GW(ButtonSondaV);
	GW(ButtonStop);
	GW(Informace);
	GW(ButtonRead);
	GW(DataShow);
	GW(DA_level);
	GW(Calibrate);
	GW(Open);
	GW(OpenWindow);
	
#undef GW
	
	//MainWindow = GTK_WIDGET(gtk_builder_get_object(gtkBuilder, "OpenWindow"));
	//MainWindow = GTK_WIDGET(gtk_builder_get_object(gtkBuilder, "MainWindow"));
	
	gtk_builder_connect_signals(gtkBuilder, dataSte);
	
	g_signal_new("analog", G_TYPE_OBJECT, G_SIGNAL_RUN_FIRST, 0,
				NULL, NULL, g_cclosure_marshal_VOID__POINTER,
				G_TYPE_NONE, 1, G_TYPE_POINTER);
        
    // Sending data to mcu
    g_signal_new("dataReceive", G_TYPE_OBJECT, G_SIGNAL_RUN_FIRST, 0,
				NULL, NULL, g_cclosure_marshal_VOID__POINTER,
				G_TYPE_NONE, 1, G_TYPE_POINTER);
	g_signal_new("stopReceive", G_TYPE_OBJECT, G_SIGNAL_RUN_FIRST, 0,
				NULL, NULL, g_cclosure_marshal_VOID__POINTER,
				G_TYPE_NONE, 1, G_TYPE_POINTER);
											
  /* TODO nadefinovat nove sygnaly do MainWindow 
  	- prijem data a obsluha po prijeti */ 
	
	
	//g_signal_connect(G_OBJECT(dataSte->MainWindow), "dataReceive", G_CALLBACK(on_ReceiveData), picdata);
	//g_signal_connect(G_OBJECT(dataSte->ButtonStop), "stopReceive", G_CALLBACK(on_ButtonStop_clicked), picdata);
	
	g_signal_connect(G_OBJECT(dataSte->DataShow), "draw", G_CALLBACK(on_DataShow_draw), picdata);
	
	//g_signal_connect(G_OBJECT(dataSte->Konec), "activate", G_CALLBACK(gtk_main_quit), NULL);
	g_signal_connect(G_OBJECT(dataSte->About), "activate", G_CALLBACK(on_About_activate), dataSte->AboutWindow);
	
	g_signal_connect(G_OBJECT(dataSte->ButtonSondaV), "clicked", G_CALLBACK(on_ButtonSondaV_clicked), picdata);
	g_signal_connect(G_OBJECT(dataSte->ButtonOpV), "clicked", G_CALLBACK(on_ButtonOpV_clicked), picdata);
	g_signal_connect(G_OBJECT(dataSte->ButtonDAC), "clicked", G_CALLBACK(on_ButtonDAC_clicked), picdata);
	g_signal_connect(G_OBJECT(dataSte->ButtonOpOut), "clicked", G_CALLBACK(on_ButtonOpOut_clicked), picdata);
	g_signal_connect(G_OBJECT(dataSte->ButtonSondaOut), "clicked", G_CALLBACK(on_ButtonSondaOut_clicked), picdata);
	g_signal_connect(G_OBJECT(dataSte->ButtonNormal), "clicked", G_CALLBACK(on_ButtonNormal_clicked), picdata);
	g_signal_connect(G_OBJECT(dataSte->ButtonNastav), "clicked", G_CALLBACK(on_ButtonNastav_clicked), picdata);
	g_signal_connect(G_OBJECT(dataSte->ButtonAutoOffset), "clicked", G_CALLBACK(on_ButtonAutoOffset_clicked), picdata);
	g_signal_connect(G_OBJECT(dataSte->ButtonStop), "clicked", G_CALLBACK(on_ButtonStop_clicked), picdata);
	g_signal_connect(G_OBJECT(dataSte->ButtonRead), "clicked", G_CALLBACK(on_ButtonRead_clicked), picdata);
	g_signal_connect(G_OBJECT(dataSte->Informace), "activate", G_CALLBACK(on_Informace_activate), picdata);
	g_signal_connect(G_OBJECT(dataSte->Calibrate), "clicked", G_CALLBACK(on_Calibrate_clicked), picdata);
	
	// Data to mcu
	g_signal_connect(G_OBJECT(dataSte->SetOffset), "value-changed", G_CALLBACK(on_SetOffset_value_changed), picdata);
	g_signal_connect(G_OBJECT(dataSte->SetVzorekT), "value-changed", G_CALLBACK(on_SetVzorekT_value_changed), picdata);
	g_signal_connect(G_OBJECT(dataSte->SetVzorekN), "value-changed", G_CALLBACK(on_SetVzorekN_value_changed), picdata);
	g_signal_connect(G_OBJECT(dataSte->UrovenZapal), "value-changed", G_CALLBACK(on_UrovenZapal_value_changed), picdata);
	g_signal_connect(G_OBJECT(dataSte->UrovenSvar), "value-changed", G_CALLBACK(on_UrovenSvar_value_changed), picdata);
	g_signal_connect(G_OBJECT(dataSte->DA_level), "value-changed", G_CALLBACK(on_DA_level_value_changed), picdata);
	
	/*
	g_signal_connect(G_OBJECT(dataSte->SetOffset), "draw", G_CALLBACK(on_SetOffset_draw), picdata);
	g_signal_connect(G_OBJECT(dataSte->SetVzorekT), "draw", G_CALLBACK(on_SetVzorekT_draw), picdata);
	g_signal_connect(G_OBJECT(dataSte->SetVzorekN), "draw", G_CALLBACK(on_SetVzorekN_draw), picdata);
	g_signal_connect(G_OBJECT(dataSte->UrovenZapal), "draw", G_CALLBACK(on_UrovenZapal_draw), picdata);
	g_signal_connect(G_OBJECT(dataSte->UrovenSvar), "draw", G_CALLBACK(on_UrovenSvar_draw), picdata);
	*/
	g_signal_connect(G_OBJECT(dataSte->HodnotaAnalog), "draw", G_CALLBACK(on_HodnotaAnalog_activated), picdata);
	g_signal_connect(G_OBJECT(dataSte->HodnotaOffset), "draw", G_CALLBACK(on_HodnotaOffset_activated), picdata);
	
	gtk_widget_set_events(dataSte->DataShow, gtk_widget_get_events(dataSte->DataShow) | GDK_POINTER_MOTION_MASK);
	g_signal_connect(G_OBJECT(dataSte->DataShow), "motion-notify-event", G_CALLBACK(on_DataShow_event), picdata);
	
	//gtk_widget_set_events(dataSte->SetOffset, gtk_widget_get_events(dataSte->SetOffset) | GDK_EXPOSURE_MASK);
	//g_signal_connect(G_OBJECT(dataSte->DataShow), "expose-event", G_CALLBACK(on_DataShow_expose_event), picdata);
	
	//picdata->Actual = MainWindow;
	/* Pocatecni hodnoty */
	picdata->picOffset = (uint8_t) 20;
	picdata->picAnVtime = (uint8_t) 10;
	picdata->picAnVnum = (uint8_t) 100;
	picdata->picUzapal = (uint16_t) 0;
	picdata->picUsvar = (uint16_t) 70;
	picdata->picDA = (uint16_t) 231;
	(picdata->picCalmState[0]) = (uint16_t) 600;
	(picdata->picCalmState[1]) = (uint16_t) 660;
	picdata->AnalogValueOp = (uint16_t) 0;
	picdata->picFiltr = FALSE;
	/*
	char itoc[10];
    sprintf(itoc, "%i", (picdata->AnalogValueOp));
    
	gtk_entry_set_text(GTK_ENTRY(dataSte->HodnotaAnalog), itoc);
	* */
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(dataSte->SetOffset), picdata->picOffset);	
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(dataSte->SetVzorekT), picdata->picAnVtime);	
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(dataSte->SetVzorekN), picdata->picAnVnum);	
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(dataSte->UrovenZapal), picdata->picUzapal);	
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(dataSte->UrovenSvar), picdata->picUsvar);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(dataSte->DA_level), picdata->picDA);
	
	OpenPort();
	if (gfd == -1)
	{
		printf("Bad pointer \n");
	}	
	g_object_unref(G_OBJECT(gtkBuilder));
	gtk_widget_show(dataSte->MainWindow);
	gtk_main();
	
	// Free any allocated data

    for (i = 0; i < TIME_MEM; i++){
        g_slice_free1(sizeof(uint16_t), Optable[i]);	
    }
    
	g_slice_free(ChData, dataSte);
	g_slice_free(struct PicData, picdata);
    
    	
	// serial port close
	close(gfd);
	
	return 0;
}

