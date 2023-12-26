#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <esp_system.h>

#include "esp_err.h"
#include "esp_log.h"

#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/api.h"
#include "errno.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

#define CONFIG_CAMERA_MODULE_ESP_S3_EYE 1

#include "esp_camera.h"
#include "camera_pins.h"
#include "file_serving_example_common.h"

static const char *TAG = "video_udp";

#define CONFIG_XCLK_FREQ 20000000 

esp_err_t init_camera(void)
{
    camera_config_t camera_config = {
        .pin_pwdn  = CAMERA_PIN_PWDN,
        .pin_reset = CAMERA_PIN_RESET,
        .pin_xclk = CAMERA_PIN_XCLK,
        .pin_sccb_sda = CAMERA_PIN_SIOD,
        .pin_sccb_scl = CAMERA_PIN_SIOC,

        .pin_d7 = CAMERA_PIN_D7,
        .pin_d6 = CAMERA_PIN_D6,
        .pin_d5 = CAMERA_PIN_D5,
        .pin_d4 = CAMERA_PIN_D4,
        .pin_d3 = CAMERA_PIN_D3,
        .pin_d2 = CAMERA_PIN_D2,
        .pin_d1 = CAMERA_PIN_D1,
        .pin_d0 = CAMERA_PIN_D0,
        .pin_vsync = CAMERA_PIN_VSYNC,
        .pin_href = CAMERA_PIN_HREF,
        .pin_pclk = CAMERA_PIN_PCLK,

        .xclk_freq_hz = CONFIG_XCLK_FREQ,
        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,

        .pixel_format = PIXFORMAT_JPEG,
        .frame_size = FRAMESIZE_VGA,

        .jpeg_quality = 10,
        .fb_count = 1,
        .grab_mode = CAMERA_GRAB_WHEN_EMPTY};//CAMERA_GRAB_LATEST. Sets when buffers should be filled
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK)
    {
        return err;
    }
    return ESP_OK;
}

#define USE_NETCONN 1
#if USE_NETCONN
struct netconn *camera_conn;
struct ip_addr peer_addr;
#else
static int camera_sock;
static struct sockaddr_in senderinfo;
#endif

static void video_stream_task(void *param){
    size_t packet_max = 32768;
    #if USE_NETCONN
        struct netbuf *txbuf = netbuf_new();
    #else
        senderinfo.sin_port = htons(55556);
    #endif

    camera_fb_t * fb = NULL;
    uint32_t frame_count = 0;
    uint32_t frame_size_sum = 0;

    static int64_t last_frame = 0;
    if(!last_frame) {
        last_frame = esp_timer_get_time();
    }

    while(true){
        fb = esp_camera_fb_get();
        if (!fb) {
            ESP_LOGE(TAG, "Camera capture failed");
            continue;
        }

        uint8_t *buf = fb->buf;
        size_t total = fb->len;
        size_t send = 0;

        for (; send < total;)
        {
            size_t txlen = total - send;
            if (txlen > packet_max)
            {
                txlen = packet_max;
            }
#if USE_NETCONN
            err_t err = netbuf_ref(txbuf, &buf[send], txlen);
            if (err == ERR_OK)
            {
                err = netconn_sendto(camera_conn, txbuf, &peer_addr, 55556);
                if (err == ERR_OK)
                {
                    send += txlen;
                }
                else if (err == ERR_MEM)
                {
                    ESP_LOGW(TAG, "netconn_sendto ERR_MEM");
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
                else
                {
                    ESP_LOGE(TAG, "netconn_sendto err %d", err);
                    break;
                }
            }
            else
            {
                ESP_LOGE(TAG, "netbuf_ref err %d", err);
                break;
            }
#else
            int err = sendto(camera_sock, &buf[send], txlen, 0, (struct sockaddr *)&senderinfo, sizeof(senderinfo));
            if (err < 0)
            {
                ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                if (errno == ENOMEM)
                {
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
                else
                {
                    send = total;
                }
            }
            else if (err == 0)
            {
                vTaskDelay(pdMS_TO_TICKS(10));
            }
            else
            {
                send += err;
            }
#endif
        }            
        esp_camera_fb_return(fb);

        if (buf[0] == 0xff && buf[1] == 0xd8 && buf[2] == 0xff)
        {
            frame_count++;
        }
        frame_size_sum += total;

        int64_t end = esp_timer_get_time();
        int64_t pasttime = end - last_frame;
        if (pasttime > 1000000)
        {
            last_frame = end;
            float adj = 1000000.0 / (float)pasttime;
            float mbps = (frame_size_sum * 8.0 * adj) / 1024.0 / 1024.0;
            ESP_LOGI(TAG, "%.1f fps %.3f Mbps", frame_count * adj, mbps);
            frame_count = 0;
            frame_size_sum = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

esp_err_t start_stream(void){
#if USE_NETCONN
    camera_conn = netconn_new(NETCONN_UDP);
    if (camera_conn == NULL)
    {
        ESP_LOGE(TAG, "netconn_new err");
        return ESP_FAIL;
    }

    err_t err = netconn_bind(camera_conn, IPADDR_ANY, 55555);
    if (err != ERR_OK)
    {
        ESP_LOGE(TAG, "netconn_bind err %d", err);
        netconn_delete(camera_conn);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Wait a trigger...");
    while (1)
    {
        struct netbuf *rxbuf;
        err = netconn_recv(camera_conn, &rxbuf);
        if (err == ERR_OK)
        {
            uint8_t *data;
            u16_t len;
            netbuf_data(rxbuf, (void **)&data, &len);
            if (len)
            {
                ESP_LOGI(TAG, "netconn_recv %d", len);
                if (data[0] == 0x55)
                {
                    peer_addr = *netbuf_fromaddr(rxbuf);
                    ESP_LOGI(TAG, "peer %x", (unsigned int) peer_addr.u_addr.ip4.addr);

                    ESP_LOGI(TAG, "Trigged!");
                    netbuf_delete(rxbuf);
                    break;
                }
            }
            netbuf_delete(rxbuf);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
#else
    camera_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (camera_sock < 0)
    {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        return;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(55555);
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(camera_sock, (struct sockaddr *)&addr, sizeof(addr));

    socklen_t addrlen = sizeof(senderinfo);
    char buf[128];
    ESP_LOGI(TAG, "Wait a trigger...");
    while (1)
    {
        int n = recvfrom(camera_sock, buf, sizeof(buf) - 1, 0, (struct sockaddr *)&senderinfo, &addrlen);
        if (n > 0 && buf[0] == 0x55)
        {
            break;
        }
        ESP_LOGI(TAG, "recvfrom %d", n);
    }
#endif
    
    if (xTaskCreatePinnedToCore(&video_stream_task, "camera_tx", 4096, NULL, 10, NULL, tskNO_AFFINITY) != pdPASS){
        ESP_LOGE(TAG, "Failed to create video stream task");
        return ESP_FAIL;
    } else{
        return ESP_OK;
    }
   
}