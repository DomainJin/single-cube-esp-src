#define DELAY_TIME_MS 100
#define FREQ 20000000
#define intervalSend 10


unsigned int touch_value;
unsigned char raw_touch;


// Bi?n d? nh?n ngu?ng t? ESP32
char uart_buffer[20];
unsigned char uart_index = 0;
unsigned int sendCount = 0;


#define NUM_CALIB_SAMPLE 50
unsigned int NGUONG_CHAM = 0;
unsigned int delta = 100;
unsigned long total = 0;
unsigned int sample;
unsigned char txt_calib[12];
unsigned int avg;
unsigned int send_enabled;

void calib_touch_threshold() {
    unsigned char i;
    UART1_Write_Text("Calib start\r\n");
    delay_ms(100);

    for(i = 0; i < NUM_CALIB_SAMPLE; i++) {
        C1CH1_BIT=0; C1CH0_BIT=1; C2CH1_BIT=0; C2CH0_BIT=1;
        Tmr1L=0;
        Tmr1H=0;
        delay_ms(DELAY_TIME_MS);
        sample=Tmr1L + Tmr1H * 255;
        total += sample;

//        WordToStr(sample, txt_calib);
//        UART1_Write_Text("Sample: ");
//        UART1_Write_Text(txt_calib);
//        UART1_Write_Text("\r\n");
//        delay_ms(50);  // Thêm delay gi?a các l?n g?i
    }

    delay_ms(100);
    WordToStr(total, txt_calib);
    UART1_Write_Text("Total: ");
    UART1_Write_Text(txt_calib);
    UART1_Write_Text("\r\n");

    delay_ms(100);
    avg = total / NUM_CALIB_SAMPLE;
    WordToStr(avg, txt_calib);
    UART1_Write_Text("Avg: ");
    UART1_Write_Text(txt_calib);
    UART1_Write_Text("\r\n");

    delay_ms(100);
    NGUONG_CHAM = avg - delta;
    WordToStr(NGUONG_CHAM, txt_calib);
    UART1_Write_Text("Calib threshold: ");
    UART1_Write_Text(txt_calib);
    UART1_Write_Text("\r\n");
    delay_ms(100);
}

unsigned char txt[7];



void sendTouchSensing(){
  WordToStr(touch_value, txt);
  UART1_Write_Text("Val:");
  UART1_Write_Text(txt);
  UART1_Write_Text("\r\n");

  WordToStr(NGUONG_CHAM, txt);
  UART1_Write_Text("Thr:");
  UART1_Write_Text(txt);
  UART1_Write_Text("\r\n");

  WordToStr(raw_touch, txt);
  UART1_Write_Text("Stt:");
  UART1_Write_Text(txt);
  UART1_Write_Text("\r\n");
}

void substr_from2_to_n(char* input, char* out, int n) {
    int j = 0;
    int i;
    for (i = 2; i < n; i++) {
        out[j++] = input[i];
    }
    out[j] = '\0'; // K?t thúc chu?i
}

unsigned int UartEnable = 1;
unsigned int CalibEnable = 0;
char rev[10];
unsigned int rev_index = 2;
char rev_temp[7];
void process_uart_data() {
    if (UART1_Data_Ready()==1) {
        char received_char = UART1_Read();

        if(received_char == 'E'){
        UartEnable = 1;
        }
        else if (received_char == 'D'){
        UartEnable = 0;
        }
        else if (received_char == 'A'){
        CalibEnable = 1;
        }
        else if (received_char == 'T'){
        UART1_Read_Text(rev, "X", 10);    // reads text until 'OK' is found
//        UART1_Write_Text(rev);
        delay_ms(100);
        for(rev_index = 2;rev_index<=9;rev_index++){
          rev_temp[rev_index-2]=rev[rev_index];
        }
        rev_temp[rev_index-2] = '\0';
        UART1_Write_Text(rev_temp);

        NGUONG_CHAM = StrToInt(rev_temp);;
        }
        // X? lý l?nh thi?t l?p ngu?ng THR:XXXX
}
}



void main()
{
 trisd=0;
 portd=0;
 TRISC.B0 = 1;
 PORTC.B0 = 0;
 TRISA=0B11011111;


 ANSEL=0b11011111;




 ////////////////////////////////////////////////////mtouch/////////////
 CM1CON0=0B10010100; //BAT CAPARATOR,
                    //0
                    //KO XUAT RA CHAN C1OUT
                    //DAO NGO RA
                    //0
                    //NOI VAO VREF
                    //CHON KENH 0


VRCON=0B10101111;   //BAT REF
                    //NGAT KET NOI CHAN VREF
                    //BAT CHIA 24
                    //CHON V LA AP NGUON
                    //VREF=2/3VDD


SRCON=0B11110000;   //CHAN C2OUT GAN VAO CHAN Q BU
                    //CHAN C1OUT GAN VAO CHAN Q
                    //ENABLE NGO VAO CAPARATOR 1
                    //ENABLE NGO VAO CAPARATOR 2
                    //DISSABLE CHAN RESET HARDWARE
                    //0
                    //0


CM2CON0=0B10100000; //BAT CAPARATOR
                    //0
                    //KET NOI NGO RA VAO CHAN C2OUT
                    //KO DAO NGO RA
                    //0
                    //KOI NOI RA CHAN C2IN+
                    //CHON KENH 0


CM2CON1=0B00110010; //KHONG DAO
                    //KHONG DAO
                    //CHON CHAN VAO LA C1CREF
                    //CHON CHAN VAO LA C2VREF
                    //0
                    //0


T1CON=0B10000111;   //ENABLE CHO TIMER1
                    //EMABLE CHO TIMER1
                    //CHIA TAN   4
                    //CHIA TAN
                    //TAT DAO DONG LP
                    //KO CHON SYNCHRONIZE
                    //CHON NGUON XONG LA CHAN T1CKI\
                    //BAT TIMER1
delay_ms(500);


TRISB=0x00;
ANSELH=0x00;

TRISC.B6 = 0;



TRISC.B7 = 1;


UART1_Init(115200);
UART1_Write_Text("From Pic With Luv");

delay_ms(2000);
calib_touch_threshold();
delay_ms(2000);
 while(1)
 {
        // X? lý d? li?u UART t? ESP32
        process_uart_data();
//        process_uart_data();

        // Ðo giá tr? c?m ?ng
        C1CH1_BIT=0; C1CH0_BIT=1; C2CH1_BIT=0; C2CH0_BIT=1; // Kênh 0
        Tmr1L = 0;
        Tmr1H = 0;
        delay_ms(DELAY_TIME_MS);
        touch_value = Tmr1L + Tmr1H * 255;

        // C?p nh?t raw_touch
        if(touch_value <= NGUONG_CHAM)
            raw_touch = 1;
        else
            raw_touch = 0;
        // G?i d? li?u m?i 20 vòng l?p (~1000ms)

        if(UartEnable)sendTouchSensing();
        if(CalibEnable){
        total=0;
        UART1_Write_Text("Auto Calib...");
        delay_ms(1000);
        calib_touch_threshold();
        delay_ms(1000);
        CalibEnable = 0;
        }


    }    // while




} // void