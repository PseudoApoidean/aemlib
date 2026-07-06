#include "aemlib/aemlib.h"
#include "core.h"
#include "aemlib/log.h"
#include "aemlib/status.h"
#include "aemlib/config.h"

// DEFINITIONS ------------------------

/* Internal helper: validate config */
static aemlib_status_t validate_config(const aemlib_core_config_t *cfg) {
    if (!cfg) {
        return AEMLIB_STATUS(AEMLIB_LAYER_GENERAL, AEMLIB_CODE_INVALID_ARG);
    }
    if (!cfg->tx_buffer || cfg->tx_buffer_size == 0) {
        return AEMLIB_STATUS(AEMLIB_LAYER_GENERAL, AEMLIB_CODE_INVALID_ARG);
    }
    if (!cfg->rx_buffer || cfg->rx_buffer_size == 0) {
        return AEMLIB_STATUS(AEMLIB_LAYER_GENERAL, AEMLIB_CODE_INVALID_ARG);
    }

    /* Validate transport/time/storage interfaces */
    if (aemlib_transport_validate(&cfg->transport) != AEMLIB_STATUS_OK) {
        return AEMLIB_STATUS(AEMLIB_LAYER_GENERAL, AEMLIB_CODE_INVALID_ARG);
    }
    if (aemlib_time_validate(&cfg->time) != AEMLIB_STATUS_OK) {
        return AEMLIB_STATUS(AEMLIB_LAYER_GENERAL, AEMLIB_CODE_INVALID_ARG);
    }

    return AEMLIB_STATUS_OK;
}

// DECLARATIONS -----------------------

static aemlib_status_t apply_config(aemlib_client_t *client, const aemlib_core_config_t *config);
static aemlib_status_t handle_state_disconnected(aemlib_client_t *client);
static aemlib_status_t handle_state_connecting(aemlib_client_t *client);
static aemlib_status_t handle_state_mqtt_connect_sent(aemlib_client_t *client);
static aemlib_status_t handle_state_mqtt_connected(aemlib_client_t *client);
static aemlib_status_t handle_state_mqtt_disconnecting(aemlib_client_t *client);

// IMPLEMENTATION ---------------------

aemlib_status_t aemlib_core_init(aemlib_client_t *client,
                                 const aemlib_core_config_t *config)
{
    if (!client) {
        return AEMLIB_STATUS(AEMLIB_LAYER_GENERAL, AEMLIB_CODE_INVALID_ARG);
    }

    aemlib_status_t status = validate_config(config);
    AEMLIB_CHECK_STATUS(status);

    status = apply_config(client, config);
    AEMLIB_CHECK_STATUS(status);

    client->packet_id = 1;

    client->state = AEMLIB_STATE_DISCONNECTED;
    client->last_activity_ms = aemlib_time_now(&client->time);

    AEMLIB_LOG_INFO(AEMLIB_LOG_MODULE_CORE, "core initialized");

    return AEMLIB_STATUS_OK;
}

aemlib_status_t aemlib_core_poll(aemlib_client_t *client)
{
    if (!client) {
        return AEMLIB_STATUS(AEMLIB_LAYER_GENERAL, AEMLIB_CODE_INVALID_ARG);
    }

    switch (client->state) {

    case AEMLIB_STATE_DISCONNECTED:
        return handle_state_disconnected(client);

    case AEMLIB_STATE_CONNECTING:
        return handle_state_connecting(client);

    case AEMLIB_STATE_MQTT_CONNECT_SENT:
        return handle_state_mqtt_connect_sent(client);

    case AEMLIB_STATE_MQTT_CONNECTED:
        return handle_state_mqtt_connected(client);

    case AEMLIB_STATE_MQTT_DISCONNECTING:
        return handle_state_mqtt_disconnecting(client);

    default:
        AEMLIB_LOG_ERROR(AEMLIB_LOG_MODULE_CORE, "invalid state: %d", client->state);
        return AEMLIB_STATUS(AEMLIB_LAYER_GENERAL, AEMLIB_CODE_STATE_INVALID);
    }
}

aemlib_status_t aemlib_core_connect(aemlib_client_t *client)
{
    if (!client) {
        return AEMLIB_STATUS(AEMLIB_LAYER_GENERAL, AEMLIB_CODE_INVALID_ARG);
    }

    if (client->state != AEMLIB_STATE_DISCONNECTED) {
        return AEMLIB_STATUS(AEMLIB_LAYER_GENERAL, AEMLIB_CODE_STATE_INVALID);
    }

    AEMLIB_LOG_INFO(AEMLIB_LOG_MODULE_CORE, "starting connection");

    client->state = AEMLIB_STATE_CONNECTING;
    return AEMLIB_STATUS_OK;
}

aemlib_status_t aemlib_core_disconnect(aemlib_client_t *client)
{
    if (!client) {
        return AEMLIB_STATUS(AEMLIB_LAYER_GENERAL, AEMLIB_CODE_INVALID_ARG);
    }

    if (client->state != AEMLIB_STATE_MQTT_CONNECTED) {
        return AEMLIB_STATUS(AEMLIB_LAYER_GENERAL, AEMLIB_CODE_STATE_INVALID);
    }

    AEMLIB_LOG_INFO(AEMLIB_LOG_MODULE_CORE, "starting disconnect");

    client->state = AEMLIB_STATE_MQTT_DISCONNECTING;
    return AEMLIB_STATUS_OK;
}

uint16_t aemlib_core_next_packet_id(aemlib_client_t *client)
{
    if (!client) {
        return 1;
    }

    client->packet_id++;
    if (client->packet_id == 0) {
        client->packet_id = 1;
    }
    return client->packet_id;
}

// STATE HANDLERS ---------------------

static aemlib_status_t handle_state_disconnected(aemlib_client_t *client)
{
    /* Nothing to do until user calls connect() */
    return AEMLIB_STATUS_OK;
}

static aemlib_status_t handle_state_connecting(aemlib_client_t *client)
{
    AEMLIB_LOG_DEBUG(AEMLIB_LOG_MODULE_CORE, "Connecting...");

    size_t out = 0;
    aemlib_status_t status = aemlib_transport_connect(&client->transport);

    if (status == AEMLIB_STATUS_OK) {
        AEMLIB_LOG_INFO(AEMLIB_LOG_MODULE_CORE, "transport connected");
        client->state = AEMLIB_STATE_MQTT_CONNECT_SENT;
        return AEMLIB_STATUS_OK;
    }

    if (AEMLIB_STATUS_CODE(status) == AEMLIB_CODE_WOULD_BLOCK) {
        return AEMLIB_STATUS_OK;
    }

    AEMLIB_LOG_ERROR(AEMLIB_LOG_MODULE_CORE, "transport connect failed");
    client->state = AEMLIB_STATE_DISCONNECTED;
    return status;
}

static aemlib_status_t handle_state_mqtt_connect_sent(aemlib_client_t *client)
{
    /* TODO: read CONNACK */
    AEMLIB_LOG_DEBUG(AEMLIB_LOG_MODULE_CORE, "waiting for CONNACK");

    /* Placeholder: assume success for now */
    client->state = AEMLIB_STATE_MQTT_CONNECTED;
    AEMLIB_LOG_INFO(AEMLIB_LOG_MODULE_CORE, "MQTT connected");

    return AEMLIB_STATUS_OK;
}

static aemlib_status_t handle_state_mqtt_connected(aemlib_client_t *client)
{
    /* TODO: handle PUBLISH, PINGRESP, keepalive, etc. */
    return AEMLIB_STATUS_OK;
}

/**
 * @brief Apply the user-provided configuration to the client instance.
 * 
 * @param client Client instance (must be non-NULL)
 * @param config User provided configuration (must be non-NULL)
 */
static aemlib_status_t apply_config(aemlib_client_t *client, const aemlib_core_config_t *config)
{
    if (!client || !config) {
        return AEMLIB_STATUS(AEMLIB_LAYER_GENERAL, AEMLIB_CODE_INVALID_ARG);
    }

    client->transport             = config->transport;
    client->time                  = config->time;
    client->storage               = config->storage;

    client->tx_buffer             = config->tx_buffer;
    client->tx_buffer_size        = config->tx_buffer_size;

    client->rx_buffer             = config->rx_buffer;
    client->rx_buffer_size        = config->rx_buffer_size;

    client->keepalive_interval_ms = config->keepalive_interval_ms;

    client->on_message            = config->on_message;
    client->on_message_ctx        = config->on_message_ctx;

    return AEMLIB_STATUS_OK;
}

static aemlib_status_t handle_state_mqtt_disconnecting(aemlib_client_t *client)
{
    /* TODO: send DISCONNECT packet */
    AEMLIB_LOG_INFO(AEMLIB_LOG_MODULE_CORE, "MQTT disconnected");

    aemlib_transport_disconnect(&client->transport);
    client->state = AEMLIB_STATE_DISCONNECTED;

    return AEMLIB_STATUS_OK;
}
