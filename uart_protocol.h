#ifndef UART_PROTOCOL_H
#define UART_PROTOCOL_H

#include "stdio.h"

#define ENABLE 1
#define DISABLE 0

#define UART_RECV_BUF_LMT         128
#define DATA_PROCESS_LMT          300
#define UART_SEND_BUF_LMT         48

#define PROTOCOL_VER            0x01                   //协议版本号，协议帧可以加上版本校验
#define PROTOCOL_HEAD           0x09                   //非数据固定协议头长度
#define FRAME_FIRST             0x53
#define FRAME_SECOND            0x59

#define FRAME_END1              0x54
#define FRAME_END0              0x43

//=============================================================================
//帧的字节顺序
//=============================================================================
#define         HEAD_FIRST                      0
#define         HEAD_SECOND                     1
#define         FRAME_CTRL_BYTE                 2
#define         FRAME_TYPE                      3
#define         LENGTH_HIGH                     4
#define         LENGTH_LOW                      5
#define         DATA_START                      6

//=============================================================================
//数据帧控制字类型
//=============================================================================
#define CTRL_SYS_FUNC           0x01
#define CTRL_PRODUCT_INFO       0x02
#define CTRL_OTA                0x03
#define CTRL_WORK_STATUS        0x05
#define CTRL_RANGE_INFO         0x07
#define CTRL_BODY_DET           0x80
#define CTRL_BREATHE_DET        0x81
#define CTRL_SLEEPING_DET       0x84
#define CTRL_HEARTBEAT_DET      0x85


//=============================================================================
//数据帧命令字类型
//=============================================================================
#define CMD_RADAR_BEATDATA     0x01
#define CMD_PRODUCT_INFO       0x02
#define CMD_OTA                0x03
#define CMD_WORK_STATUS        0x04
#define CMD_RANGE_INFO         0x05



class UART_Protocol
{
public:
    UART_Protocol();
    ~UART_Protocol();

    unsigned char stop_update_flag;

    volatile unsigned char *queue_in;
    volatile unsigned char *queue_out;

    volatile unsigned char uart_rx_buf[PROTOCOL_HEAD + UART_RECV_BUF_LMT];  //串口接收缓存
    unsigned char uart_tx_buf[PROTOCOL_HEAD + UART_SEND_BUF_LMT];           //串口发送缓存
    unsigned char data_process_buf[PROTOCOL_HEAD + DATA_PROCESS_LMT];       //串口数据处理缓存


    typedef struct{
        unsigned char breathe_rate;
        unsigned char breathe_wave[5];
        unsigned char breathe_result;

        unsigned char module_blink;

        unsigned char heartbeat_rate;
        unsigned char heartbeat_wave[5];

        bool human_det;
        short human_dist;
        unsigned char movement_det;
        unsigned char movement_value;
        unsigned char coord[6];

        char iswakeup;
        char isinbed;

        bool exist_sw;
        bool heart_sw;
        bool breathe_sw;
        bool sleep_sw;

        struct{
            unsigned char p_number[9];
            unsigned char pid[9];
            unsigned char hw_number[9];
            unsigned char fw_ver[6];
        }productInfo;

    } RadarData;
    volatile RadarData *radarData;

    int UART_Recv(unsigned char *value, unsigned short len);
    unsigned char* UART_Send(unsigned char fr_ctrl, unsigned char fr_type, unsigned short len, unsigned char *data);

    void UART_Service(void);
    int data_Parse(unsigned short offset);

    unsigned char get_check_sum(unsigned char *pack, unsigned short pack_len);
    void *my_memcpy(void *dest, const void *src, unsigned short count);
    unsigned char get_queue_total_data(void);
    unsigned char Queue_Read_Byte(void);

};

#endif // UART_PROTOCOL_H
