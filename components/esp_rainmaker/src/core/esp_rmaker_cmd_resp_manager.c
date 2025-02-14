// Copyright 2020 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include <sdkconfig.h>
#include <esp_log.h>
#include <string.h>
#include <esp_rmaker_mqtt.h>
#include <esp_rmaker_cmd_resp.h>
#include "esp_rmaker_internal.h"

#define TO_NODE_TOPIC_SUFFIX            "to-node"
#define FROM_NODE_TOPIC_SUFFIX          "from-node"

static const char *TAG = "esp_rmaker_cmd_resp";

#ifdef CONFIG_ESP_RMAKER_CMD_RESP_TEST_ENABLE

/* These are for testing purpose only */
static void esp_rmaker_resp_callback(const char *topic, void *payload, size_t payload_len, void *priv_data)
{
    esp_rmaker_cmd_resp_parse_response(payload, payload_len, priv_data);

}

esp_err_t esp_rmaker_test_cmd_resp(const void *cmd, size_t cmd_len, void *priv_data)
{
    if (!cmd) {
        ESP_LOGE(TAG, "No command data to send.");
        return ESP_ERR_INVALID_ARG;
    }
    char publish_topic[100];
    snprintf(publish_topic, sizeof(publish_topic), "node/%s/%s", esp_rmaker_get_node_id(), TO_NODE_TOPIC_SUFFIX);
    return esp_rmaker_mqtt_publish(publish_topic, cmd, cmd_len, RMAKER_MQTT_QOS1, NULL);
}

static esp_err_t esp_rmaker_cmd_resp_test_enable(void)
{
    char subscribe_topic[100];
    snprintf(subscribe_topic, sizeof(subscribe_topic), "node/%s/%s",
                esp_rmaker_get_node_id(), FROM_NODE_TOPIC_SUFFIX);
    esp_err_t err = esp_rmaker_mqtt_subscribe(subscribe_topic, esp_rmaker_resp_callback, RMAKER_MQTT_QOS1, NULL);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to subscribe to %s. Error %d", subscribe_topic, err);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Command-Response test support enabled.");
    return ESP_OK;
}

#else
esp_err_t esp_rmaker_test_cmd_resp(const void *cmd, size_t cmd_len, void *priv_data)
{
    ESP_LOGE(TAG, "Please enable CONFIG_ESP_RMAKER_CMD_RESP_TEST_ENABLE to use this.");
    return ESP_FAIL;
}
#endif /* !CONFIG_ESP_RMAKER_CMD_RESP_TEST_ENABLE */

#ifdef CONFIG_ESP_RMAKER_CMD_RESP_ENABLE

static void esp_rmaker_cmd_callback(const char *topic, void *payload, size_t payload_len, void *priv_data)
{
    void *output = NULL;
    size_t output_len = 0;
    /* Any command data received is directly sent to the command response framework and on success,
     * the response (if any) is sent back to the MQTT Broker.
     */
    if (esp_rmaker_cmd_response_handler(payload, payload_len, &output, &output_len) == ESP_OK) {
        if (output) {
            char publish_topic[100];
            snprintf(publish_topic, sizeof(publish_topic), "node/%s/%s", esp_rmaker_get_node_id(), FROM_NODE_TOPIC_SUFFIX);
            if (esp_rmaker_mqtt_publish(publish_topic, output, output_len, RMAKER_MQTT_QOS1, NULL) != ESP_OK) {
                ESP_LOGE(TAG, "Failed to publish reponse.");
            }
            free(output);
        } else {
            ESP_LOGE(TAG, "No output generated by command-response handler.");
        }
    }
}

esp_err_t esp_rmaker_cmd_response_enable(void)
{
    ESP_LOGI(TAG, "Enabling Command-Response Module.");
    char subscribe_topic[100];
    snprintf(subscribe_topic, sizeof(subscribe_topic), "node/%s/%s",
                esp_rmaker_get_node_id(), TO_NODE_TOPIC_SUFFIX);
    esp_err_t err = esp_rmaker_mqtt_subscribe(subscribe_topic, esp_rmaker_cmd_callback, RMAKER_MQTT_QOS1, NULL);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to subscribe to %s. Error %d", subscribe_topic, err);
        return ESP_FAIL;
    }
#ifdef CONFIG_ESP_RMAKER_CMD_RESP_TEST_ENABLE
    esp_rmaker_cmd_resp_test_enable();
#endif /* CONFIG_ESP_RMAKER_CMD_RESP_TEST_ENABLE */
    return ESP_OK;
}
#else
esp_err_t esp_rmaker_cmd_response_enable(void)
{
    ESP_LOGW(TAG, "Command-Response Module not enabled. Set CONFIG_ESP_RMAKER_CMD_RESP_ENABLE=y to use this.");
    return ESP_OK;
}
#endif /* !CONFIG_ESP_RMAKER_CMD_RESP_ENABLE */
