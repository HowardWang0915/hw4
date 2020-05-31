#include "mbed.h"
#include "mbed_rpc.h"
#include "fsl_port.h"
#include "fsl_gpio.h"
#include <cmath>
#define UINT14_MAX        16383
// FXOS8700CQ I2C address
#define FXOS8700CQ_SLAVE_ADDR0 (0x1E<<1) // with pins SA0=0, SA1=0
#define FXOS8700CQ_SLAVE_ADDR1 (0x1D<<1) // with pins SA0=1, SA1=0
#define FXOS8700CQ_SLAVE_ADDR2 (0x1C<<1) // with pins SA0=0, SA1=1
#define FXOS8700CQ_SLAVE_ADDR3 (0x1F<<1) // with pins SA0=1, SA1=1
// FXOS8700CQ internal register addresses
#define FXOS8700Q_STATUS 0x00
#define FXOS8700Q_OUT_X_MSB 0x01
#define FXOS8700Q_OUT_Y_MSB 0x03
#define FXOS8700Q_OUT_Z_MSB 0x05
#define FXOS8700Q_M_OUT_X_MSB 0x33
#define FXOS8700Q_M_OUT_Y_MSB 0x35
#define FXOS8700Q_M_OUT_Z_MSB 0x37
#define FXOS8700Q_WHOAMI 0x0D
#define FXOS8700Q_XYZ_DATA_CFG 0x0E
#define FXOS8700Q_CTRL_REG1 0x2A
#define FXOS8700Q_M_CTRL_REG1 0x5B
#define FXOS8700Q_M_CTRL_REG2 0x5C
#define FXOS8700Q_WHOAMI_VAL 0xC7
I2C i2c( PTD9,PTD8);
RawSerial pc(USBTX, USBRX);
RawSerial xbee(D12, D11);

EventQueue queue(32 * EVENTS_EVENT_SIZE);
Thread t;
Thread GD;  // get data
// Thread TC;  // tilt check
int m_addr = FXOS8700CQ_SLAVE_ADDR1;
// how many times has sampled
int times = 0;
// datas
float t1[3];
float xdata[200], ydata[200], zdata[200]; 
int logdata[200];
// flag for clearing the function or not
bool clear = false;
bool start = false;
// functions
void xbee_rx_interrupt(void);
void xbee_rx(void);
void reply_messange(char *xbee_reply, char *messange);
void check_addr(char *xbee_reply, char *messenger);
// accelermeter
void FXOS8700CQ_readRegs(int addr, uint8_t * data, int len);
void FXOS8700CQ_writeRegs(uint8_t * data, int len);
void GetData(void);
void How_Many_Times(Arguments *in, Reply *out);
void Plot(Arguments *in, Reply *out);
// void Tilt_Check(void);
RPCFunction Plotting(&Plot, "Plot");
RPCFunction How_Many(&How_Many_Times, "How_Many_Times");

int main(){
    pc.baud(9600);

    char xbee_reply[4];

    // XBee setting
    xbee.baud(9600);
    xbee.printf("+++");
    xbee_reply[0] = xbee.getc();
    xbee_reply[1] = xbee.getc();
    if(xbee_reply[0] == 'O' && xbee_reply[1] == 'K') {
      	pc.printf("enter AT mode.\r\n");
      	xbee_reply[0] = '\0';
      	xbee_reply[1] = '\0';
    }
    xbee.printf("ATMY 0x140\r\n");
    reply_messange(xbee_reply, "setting MY : 0x140");

    xbee.printf("ATDL 0x240\r\n");
    reply_messange(xbee_reply, "setting DL : 0x240");

    xbee.printf("ATID 0x1\r\n");
    reply_messange(xbee_reply, "setting PAN ID : 0x1");

    xbee.printf("ATWR\r\n");
    reply_messange(xbee_reply, "write config");

    xbee.printf("ATMY\r\n");
    check_addr(xbee_reply, "MY");

    xbee.printf("ATDL\r\n");
    check_addr(xbee_reply, "DL");

    xbee.printf("ATCN\r\n");
    reply_messange(xbee_reply, "exit AT mode");
    xbee.getc();

    // start
    pc.printf("start\r\n");
    t.start(callback(&queue, &EventQueue::dispatch_forever));
    GD.start(GetData);
    // TC.start(Tilt_Check);
    // Setup a serial interrupt function of receiving data from xbee
    xbee.attach(xbee_rx_interrupt, Serial::RxIrq);
}

void xbee_rx_interrupt(void)
{
    xbee.attach(NULL, Serial::RxIrq); // detach interrupt
    queue.call(&xbee_rx);
}

void xbee_rx(void)
{
    char buf[100] = {0};
    char outbuf[100] = {0};
    while(xbee.readable()){
      	for (int i=0; ; i++) {
        	char recv = xbee.getc();
      		if (recv == '\r') {
        		break;
      		}
     	buf[i] = pc.putc(recv);
    	}
    	RPC::call(buf, outbuf);
    	// pc.printf("%s\r\n", outbuf);
    	wait(0.1);
  	}
  	xbee.attach(xbee_rx_interrupt, Serial::RxIrq); // reattach interrupt
}

void reply_messange(char *xbee_reply, char *messange){
  	xbee_reply[0] = xbee.getc();
  	xbee_reply[1] = xbee.getc();
  	xbee_reply[2] = xbee.getc();
  	if(xbee_reply[1] == 'O' && xbee_reply[2] == 'K'){
    	pc.printf("%s\r\n", messange);
    	xbee_reply[0] = '\0';
    	xbee_reply[1] = '\0';
    	xbee_reply[2] = '\0';
  	}
}

void check_addr(char *xbee_reply, char *messenger){
  	xbee_reply[0] = xbee.getc();
  	xbee_reply[1] = xbee.getc();
  	xbee_reply[2] = xbee.getc();
  	xbee_reply[3] = xbee.getc();
  	pc.printf("%s = %c%c%c\r\n", messenger, xbee_reply[1], xbee_reply[2], xbee_reply[3]);
  	xbee_reply[0] = '\0';
  	xbee_reply[1] = '\0';
  	xbee_reply[2] = '\0';
  	xbee_reply[3] = '\0';
}
void FXOS8700CQ_readRegs(int addr, uint8_t * data, int len) {
    char t = addr;
    i2c.write(m_addr, &t, 1, true);
    i2c.read(m_addr, (char *)data, len);
}

void FXOS8700CQ_writeRegs(uint8_t * data, int len) {
    i2c.write(m_addr, (char *)data, len);
}
void GetData(void) {

    pc.baud(9600);

    uint8_t who_am_i, data[2], res[6];
    int16_t acc16;
    // float t[3];
    int i = 0;
    // Enable the FXOS8700Q

    FXOS8700CQ_readRegs( FXOS8700Q_CTRL_REG1, &data[1], 1);
    data[1] |= 0x01;
    data[0] = FXOS8700Q_CTRL_REG1;
    FXOS8700CQ_writeRegs(data, 2);

    // Get the slave address
    FXOS8700CQ_readRegs(FXOS8700Q_WHOAMI, &who_am_i, 1);

    // pc.printf("Here is %x\r\n", who_am_i);
    while (true) {
        
        FXOS8700CQ_readRegs(FXOS8700Q_OUT_X_MSB, res, 6);

        acc16 = (res[0] << 6) | (res[1] >> 2);
        if (acc16 > UINT14_MAX/2)
            acc16 -= UINT14_MAX;
        t1[0] = ((float)acc16) / 4096.0f;

        acc16 = (res[2] << 6) | (res[3] >> 2);
        if (acc16 > UINT14_MAX/2)
            acc16 -= UINT14_MAX;
        t1[1] = ((float)acc16) / 4096.0f;
        acc16 = (res[4] << 6) | (res[5] >> 2);
        if (acc16 > UINT14_MAX/2)
            acc16 -= UINT14_MAX;
        t1[2] = ((float)acc16) / 4096.0f;

        // pc.printf("FXOS8700Q ACC: X=%1.4f(%x%x) Y=%1.4f(%x%x) Z=%1.4f(%x%x)\r\n",
        // t[0], res[0], res[1],
        // t[1], res[2], res[3],
        // t[2], res[4], res[5]
        // );
        
        if (i < 200 && start) {
            xdata[i] = t1[0];
            ydata[i] = t1[1];
            zdata[i] = t1[2];
            if (acos(t1[2] / (sqrt(pow(t1[0], 2) + pow(t1[1], 2) + pow(t1[2], 2)))) * 180 / M_PI > 45) 
                logdata[i] = 1;
            else
                logdata[i] = 0;
            i++;
        }
        if (clear) {
            clear = false;
            times = 0;
        }
        times++;
        wait(0.1);
    }
}

// if (acos(t[2] / (sqrt(pow(t[0], 2) + pow(t[1], 2) + pow(t[2], 2)))) * 180 / M_PI > 45)
void How_Many_Times(Arguments *in, Reply *out) {
    xbee.printf(" %d\r\n", times);
    start = true;
    clear = true;
}
void Plot(Arguments *in, Reply *out) {
    for (int i = 0; i < 200; i++) {
        xbee.printf("%1.3f\r\n", xdata[i]);
    }
    for (int i = 0; i < 200; i++) {
        xbee.printf("%1.3f\r\n", ydata[i]);
    }
    for (int i = 0; i < 200; i++) {
        xbee.printf("%1.3f\r\n", zdata[i]);
    }
    pc.printf("\r\n");
    for (int i = 0; i < 200; i++) {
        xbee.printf("%d\r\n", logdata[i]);
    }
}