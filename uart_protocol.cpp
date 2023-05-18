#include "uart_protocol.h"
#include <QDebug>

UART_Protocol::UART_Protocol()
{
    queue_in = (unsigned char *)uart_rx_buf;
    queue_out = (unsigned char *)uart_rx_buf;

    radarData = (RadarData*)malloc(sizeof(RadarData));

    stop_update_flag = DISABLE;
    qDebug()<<"UART Protocol Parse Starting!";
    printf("UART Protocol Parse Start!\n");
}

UART_Protocol::~UART_Protocol()
{
    qDebug()<<"UART Protocol Parse Stoping!";
    printf("UART Protocol Parse End!\n");
}

int UART_Protocol::UART_Recv(unsigned char *value, unsigned short len)
{
while(len --) {

    if(1 == queue_out - queue_in) {
            //数据队列满
    }else if((queue_in > queue_out) && ((queue_in - queue_out) >= (long long)sizeof(uart_rx_buf))) {
            //数据队列满
    }else {
            //队列不满
            if(queue_in >= (unsigned char *)(uart_rx_buf + sizeof(uart_rx_buf))) {
                queue_in = (unsigned char *)(uart_rx_buf);
            }

            *queue_in ++ = *value;
//            qDebug()<<*value;
            value ++;
    }
}

    return 0;
}

unsigned char* UART_Protocol::UART_Send(unsigned char fr_ctrl, unsigned char fr_type, unsigned short len, unsigned char *data)
{
    unsigned char check_sum = 0, *value;
    unsigned short i = len;
    uart_tx_buf[HEAD_FIRST] = FRAME_FIRST;
    uart_tx_buf[HEAD_SECOND] = FRAME_SECOND;
    uart_tx_buf[FRAME_CTRL_BYTE] = fr_ctrl;
    uart_tx_buf[FRAME_TYPE] = fr_type;
    uart_tx_buf[LENGTH_HIGH] = len >> 8;
    uart_tx_buf[LENGTH_LOW] = len & 0xff;

    if(i > 0){
        value = data;
        for(;i > 0;i--){
            uart_tx_buf[LENGTH_LOW+i] = *value & 0xff;
            *value = *value >> 8;
        }
    }

    len += PROTOCOL_HEAD;
    check_sum = get_check_sum((unsigned char *)uart_tx_buf, len - 3);
    uart_tx_buf[len - 3] = check_sum;
    uart_tx_buf[len - 2] = FRAME_END1;
    uart_tx_buf[len - 1] = FRAME_END0;    //frame end
//    uart_write_data((unsigned char *)uart_tx_buf, len);

    return (unsigned char *)uart_rx_buf; //返回拼接好的消息帧
}

void UART_Protocol::UART_Service(void)
{
    // 线程处理服务
    static unsigned short rx_in = 0;
    unsigned short offset = 0;
    unsigned short rx_value_len = 0;             //数据帧长度

    while((rx_in < sizeof(data_process_buf)) && get_queue_total_data() > 0) {
        data_process_buf[rx_in ++] = Queue_Read_Byte();
    }

    if(rx_in < PROTOCOL_HEAD)
        return;

    while((rx_in - offset) >= PROTOCOL_HEAD) {
        if(data_process_buf[offset + HEAD_FIRST] != FRAME_FIRST) {
            offset ++;
            continue;
        }

        if(data_process_buf[offset + HEAD_SECOND] != FRAME_SECOND) {
            offset ++;
            continue;
        }


        rx_value_len = data_process_buf[offset + LENGTH_HIGH] * 0x100 + data_process_buf[offset + LENGTH_LOW] + PROTOCOL_HEAD;
        if(rx_value_len > sizeof(data_process_buf) + PROTOCOL_HEAD) {
            offset += 2;
            continue;
        }

        if((rx_in - offset) < rx_value_len) {
            break;
        }

        //数据接收完成
        if(get_check_sum((unsigned char *)data_process_buf + offset,rx_value_len - 3) != data_process_buf[offset + rx_value_len - 3]) {
            //校验出错
            //printf("crc error (crc:0x%X  but data:0x%X)\r\n",get_check_sum((unsigned char *)wifi_data_process_buf + offset,rx_value_len - 1),wifi_data_process_buf[offset + rx_value_len - 1]);
            offset += 2;
            continue;
        }

        data_Parse(offset);
        offset += rx_value_len;
    }//end while

    rx_in -= offset;
    if(rx_in > 0) {
        my_memcpy(data_process_buf,data_process_buf + offset,rx_in);
    }
}

unsigned char UART_Protocol::get_queue_total_data(void)
{
    if(queue_in != queue_out)
        return 1;
    else
        return 0;
}

unsigned char UART_Protocol::Queue_Read_Byte(void)
{
    unsigned char value =0x00;

    if(queue_out != queue_in) {
        //有数据
        if(queue_out >= (unsigned char *)(uart_rx_buf + sizeof(uart_rx_buf))) {
            //数据已经到末尾
            queue_out = (unsigned char *)(uart_rx_buf);
        }

        value = *queue_out++;
    }

    return value;
}

void *UART_Protocol::my_memcpy(void *dest, const void *src, unsigned short count)
{
    unsigned char *pdest = (unsigned char *)dest;
    const unsigned char *psrc  = (const unsigned char *)src;
    unsigned short i;

    if(dest == NULL || src == NULL) {
        return NULL;
    }

    if((pdest <= psrc) || (pdest > psrc + count)) {
        for(i = 0; i < count; i ++) {
            pdest[i] = psrc[i];
        }
    }else {
        for(i = count; i > 0; i --) {
            pdest[i - 1] = psrc[i - 1];
        }
    }

    return dest;
}

unsigned char UART_Protocol::get_check_sum(unsigned char *pack, unsigned short pack_len)
{
    unsigned short i;
    unsigned char check_sum = 0;

    for(i = 0; i < pack_len; i ++) {
        check_sum += *pack ++;
    }

    return check_sum;
}

/**
 * @brief  数据帧处理
 * @param[in] {offset} 数据起始位
 * @param[in] {result} 处理结果
 * @return Int
 */
int UART_Protocol::data_Parse(unsigned short offset)
{
    unsigned char ctrl_type = data_process_buf[offset + FRAME_CTRL_BYTE];
    unsigned char cmd_type = data_process_buf[offset + FRAME_TYPE];
    short len = data_process_buf[offset + LENGTH_HIGH]*0x100+data_process_buf[offset + LENGTH_LOW];
    short i = 0;
    switch(ctrl_type)
    {
            case CTRL_SYS_FUNC:                                     //模块系统功能
                printf("radar heartbeat");
                break;

            case CTRL_PRODUCT_INFO:                                  //产品信息
                if(0xa1 == cmd_type){
                    for(i = 0;i < len;i++){
                        radarData->productInfo.p_number[i] = (unsigned char)data_process_buf[offset + DATA_START + i];
                    }
                }else if (0xa2 == cmd_type) {
                    for(i = 0;i < len;i++){
                        radarData->productInfo.pid[i] = (unsigned char)data_process_buf[offset + DATA_START + i];
                    }
                }else if (0xa3 == cmd_type) {
                    for(i = 0;i < len;i++){
                        radarData->productInfo.hw_number[i] = (unsigned char)data_process_buf[offset + DATA_START + i];
                    }
                }else if (0xa4 == cmd_type) {
                    for(i = 0;i < len;i++){
                        radarData->productInfo.fw_ver[i] = (unsigned char)data_process_buf[offset + DATA_START + i];
                    }
                }else{
                    printf("unkown product info!");
                }
                break;

            case CTRL_OTA:                                           //OTA

                break;

            case CTRL_WORK_STATUS:

                break;

            case CTRL_RANGE_INFO:

                break;

            case CTRL_BODY_DET:
                if(0x01 == cmd_type){
                    radarData->human_det = (bool)data_process_buf[offset + DATA_START];
                }else if (0x02 == cmd_type) {
                    radarData->movement_det = (unsigned char)data_process_buf[offset + DATA_START];
                }else if (0x03 == cmd_type) {
                    radarData->movement_value = (unsigned char)data_process_buf[offset + DATA_START];
                }else if (0x04 == cmd_type) {
                    radarData->human_dist = (short)data_process_buf[offset + DATA_START]*0x100+data_process_buf[offset + DATA_START+1];
                }else if (0x00 == cmd_type || 0x80 == cmd_type) {
                    radarData->exist_sw = (bool)data_process_buf[offset + DATA_START];
                }else{
//                        printf("unkown body info!");
                }
                break;

            case CTRL_BREATHE_DET:
                if(0x02 == cmd_type){
                    radarData->breathe_rate = (unsigned char)data_process_buf[offset + DATA_START];
                }else if (0x05 == cmd_type) {
                    for(i = 0;i < len;i++){
                        radarData->breathe_wave[i] = (unsigned char)data_process_buf[offset + DATA_START+i];
                    }
                }else if (0x01 == cmd_type) {
                    radarData->breathe_result = (unsigned char)data_process_buf[offset + DATA_START];
                }else if (0x00 == cmd_type || 0x80 == cmd_type) {
                    radarData->breathe_sw = (bool)data_process_buf[offset + DATA_START];
                }else{
                    printf("unkown breathe info!");
                }
                break;

            case CTRL_SLEEPING_DET:
                if(0x02 == cmd_type){
                    radarData->iswakeup = (char)data_process_buf[offset + DATA_START];
                }else if (0x01 == cmd_type) {
                    radarData->isinbed = (char)data_process_buf[offset + DATA_START];
                }else if (0x00 == cmd_type || 0x80 == cmd_type) {
                    radarData->sleep_sw = (bool)data_process_buf[offset + DATA_START];
                }else{
                    printf("unkown sleeping info!");
                }
                break;

            case CTRL_HEARTBEAT_DET:
                if(0x02 == cmd_type){
                    radarData->heartbeat_rate = (unsigned char)data_process_buf[offset + DATA_START];
                }else if (0x05 == cmd_type) {
                    for(i = 0;i < len;i++){
                        radarData->heartbeat_wave[i] = (unsigned char)data_process_buf[offset + DATA_START+i];
                    }
                }else if (0x00 == cmd_type || 0x80 == cmd_type) {
                    radarData->heart_sw = (bool)data_process_buf[offset + DATA_START];
                }else{
                    printf("unkown heartbeat info!");
                }
                break;

            default:
                printf("unkown info detect!");
                break;
    }

    return 0;
}

