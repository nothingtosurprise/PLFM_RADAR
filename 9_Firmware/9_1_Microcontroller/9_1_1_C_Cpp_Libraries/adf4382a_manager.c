#include "adf4382a_manager.h"
#include "no_os_delay.h"
#include <stdio.h>
#include <string.h>

// External SPI handle
extern SPI_HandleTypeDef hspi4;

// Static function prototypes
static void set_chip_enable(uint8_t ce_pin, bool state);
static void set_deladj_pin(uint8_t device, bool state);
static void set_delstr_pin(uint8_t device, bool state);
static uint16_t phase_ps_to_duty_cycle(uint16_t phase_ps);

int ADF4382A_Manager_Init(ADF4382A_Manager *manager, SyncMethod method)
{
    struct adf4382_init_param tx_param, rx_param;
    int ret;
    
    if (!manager) {
        return ADF4382A_MANAGER_ERROR_INVALID;
    }
    
    // Initialize manager structure
    manager->tx_dev = NULL;
    manager->rx_dev = NULL;
    manager->initialized = false;
    manager->sync_method = method;
    manager->tx_phase_shift_ps = 0;
    manager->rx_phase_shift_ps = 0;
    
    // Initialize SPI parameters in manager
    memset(&manager->spi_tx_param, 0, sizeof(manager->spi_tx_param));
    memset(&manager->spi_rx_param, 0, sizeof(manager->spi_rx_param));

    // Setup TX SPI parameters for SPI4
    manager->spi_tx_param.device_id = ADF4382A_SPI_DEVICE_ID;
    manager->spi_tx_param.max_speed_hz = ADF4382A_SPI_SPEED_HZ;
    manager->spi_tx_param.mode = NO_OS_SPI_MODE_0;
    manager->spi_tx_param.chip_select = TX_CS_Pin;
    manager->spi_tx_param.bit_order = NO_OS_SPI_BIT_ORDER_MSB_FIRST;
    manager->spi_tx_param.platform_ops = NULL;
    manager->spi_tx_param.extra = &hspi4;
    
    // Setup RX SPI parameters for SPI4
    manager->spi_rx_param.device_id = ADF4382A_SPI_DEVICE_ID;
    manager->spi_rx_param.max_speed_hz = ADF4382A_SPI_SPEED_HZ;
    manager->spi_rx_param.mode = NO_OS_SPI_MODE_0;
    manager->spi_rx_param.chip_select = RX_CS_Pin;
    manager->spi_rx_param.bit_order = NO_OS_SPI_BIT_ORDER_MSB_FIRST;
    manager->spi_rx_param.platform_ops = NULL;
    manager->spi_rx_param.extra = &hspi4;
    
    // Configure TX parameters (10.5 GHz)
    memset(&tx_param, 0, sizeof(tx_param));
    tx_param.spi_3wire_en = 0;
    tx_param.cmos_3v3 = 1;
    tx_param.ref_freq_hz = REF_FREQ_HZ;
    tx_param.ref_div = 1;
    tx_param.ref_doubler_en = false;
    tx_param.freq = TX_FREQ_HZ;
    tx_param.id = ID_ADF4382A;
    tx_param.cp_i = 3;
    tx_param.bleed_word = 1000;
    tx_param.ld_count = 0x07;
    tx_param.spi_init = &manager->spi_tx_param;
    
    // Configure RX parameters (10.38 GHz)
    memset(&rx_param, 0, sizeof(rx_param));
    rx_param.spi_3wire_en = 0;
    rx_param.cmos_3v3 = 1;
    rx_param.ref_freq_hz = REF_FREQ_HZ;
    rx_param.ref_div = 1;
    rx_param.ref_doubler_en = false;
    rx_param.freq = RX_FREQ_HZ;
    rx_param.id = ID_ADF4382A;
    rx_param.cp_i = 4;
    rx_param.bleed_word = 1200;
    rx_param.ld_count = 0x07;
    rx_param.spi_init = &manager->spi_rx_param;
    
    // Enable chips
    set_chip_enable(TX_CE_Pin, true);
    set_chip_enable(RX_CE_Pin, true);
    no_os_udelay(1000);
    
    // Initialize DELADJ and DELSTR pins
    set_deladj_pin(0, false); // TX device
    set_deladj_pin(1, false); // RX device
    set_delstr_pin(0, false); // TX device
    set_delstr_pin(1, false); // RX device

    // Initialize TX device first
    printf("Initializing TX ADF4382A (10.5 GHz) on SPI4...\n");
    ret = adf4382_init(&manager->tx_dev, &tx_param);
    if (ret) {
        printf("TX ADF4382A initialization failed: %d\n", ret);
        set_chip_enable(TX_CE_Pin, false);
        set_chip_enable(RX_CE_Pin, false);
        return ADF4382A_MANAGER_ERROR_SPI;
    }
    
    // Small delay between initializations
    no_os_udelay(5000);
    
    // Initialize RX device
    printf("Initializing RX ADF4382A (10.38 GHz) on SPI4...\n");
    ret = adf4382_init(&manager->rx_dev, &rx_param);
    if (ret) {
        printf("RX ADF4382A initialization failed: %d\n", ret);
        adf4382_remove(manager->tx_dev);
        set_chip_enable(TX_CE_Pin, false);
        set_chip_enable(RX_CE_Pin, false);
        return ADF4382A_MANAGER_ERROR_SPI;
    }
    
    // Set output power
    adf4382_set_out_power(manager->tx_dev, 0, 12);
    adf4382_set_out_power(manager->tx_dev, 1, 12);
    adf4382_set_out_power(manager->rx_dev, 0, 12);
    adf4382_set_out_power(manager->rx_dev, 1, 12);
    
    // Enable outputs
    adf4382_set_en_chan(manager->tx_dev, 0, true);
    adf4382_set_en_chan(manager->tx_dev, 1, false);
    adf4382_set_en_chan(manager->rx_dev, 0, true);
    adf4382_set_en_chan(manager->rx_dev, 1, false);
    
    // Setup synchronization based on selected method
    if (method == SYNC_METHOD_TIMED) {
        ret = ADF4382A_SetupTimedSync(manager);
        if (ret) {
            printf("Timed sync setup failed: %d\n", ret);
        }
    } else {
        ret = ADF4382A_SetupEZSync(manager);
        if (ret) {
            printf("EZSync setup failed: %d\n", ret);
        }
    }
    
    manager->initialized = true;
    printf("ADF4382A Manager initialized with %s synchronization on SPI4\n",
           (method == SYNC_METHOD_TIMED) ? "TIMED" : "EZSYNC");
    
    return ADF4382A_MANAGER_OK;
}

int ADF4382A_SetupTimedSync(ADF4382A_Manager *manager)
{
    int ret;

    if (!manager || !manager->initialized) {
        return ADF4382A_MANAGER_ERROR_NOT_INIT;
    }
    
    printf("Setting up Timed Synchronization (60 MHz SYNCP/SYNCN)...\n");
    
    // Setup TX for timed sync
    ret = adf4382_set_timed_sync_setup(manager->tx_dev, true);
    if (ret) {
        printf("TX timed sync setup failed: %d\n", ret);
        return ret;
    }
    
    // Setup RX for timed sync
    ret = adf4382_set_timed_sync_setup(manager->rx_dev, true);
    if (ret) {
        printf("RX timed sync setup failed: %d\n", ret);
        return ret;
    }
    
    manager->sync_method = SYNC_METHOD_TIMED;
    printf("Timed synchronization configured for 60 MHz SYNCP/SYNCN\n");
    
    return ADF4382A_MANAGER_OK;
}

int ADF4382A_SetupEZSync(ADF4382A_Manager *manager)
{
    int ret;
    
    if (!manager || !manager->initialized) {
        return ADF4382A_MANAGER_ERROR_NOT_INIT;
    }
    
    printf("Setting up EZSync (SPI-based synchronization)...\n");

    // Setup TX for EZSync
    ret = adf4382_set_ezsync_setup(manager->tx_dev, true);
    if (ret) {
        printf("TX EZSync setup failed: %d\n", ret);
        return ret;
    }
    
    // Setup RX for EZSync
    ret = adf4382_set_ezsync_setup(manager->rx_dev, true);
    if (ret) {
        printf("RX EZSync setup failed: %d\n", ret);
        return ret;
    }
    
    manager->sync_method = SYNC_METHOD_EZSYNC;
    printf("EZSync configured\n");
    
    return ADF4382A_MANAGER_OK;
}

int ADF4382A_TriggerTimedSync(ADF4382A_Manager *manager)
{
    if (!manager || !manager->initialized || manager->sync_method != SYNC_METHOD_TIMED) {
        return ADF4382A_MANAGER_ERROR_NOT_INIT;
    }
    
    printf("Timed sync ready - SYNC pin will trigger synchronization\n");
    printf("Ensure 60 MHz phase-aligned clocks are present on SYNCP/SYNCN pins\n");

    return ADF4382A_MANAGER_OK;
}

int ADF4382A_TriggerEZSync(ADF4382A_Manager *manager)
{
    int ret;
    
    if (!manager || !manager->initialized || manager->sync_method != SYNC_METHOD_EZSYNC) {
        return ADF4382A_MANAGER_ERROR_NOT_INIT;
    }

    // Trigger software sync on both devices
    ret = adf4382_set_sw_sync(manager->tx_dev, true);
    if (ret) {
        printf("TX software sync failed: %d\n", ret);
        return ADF4382A_MANAGER_ERROR_SPI;
    }

    ret = adf4382_set_sw_sync(manager->rx_dev, true);
    if (ret) {
        printf("RX software sync failed: %d\n", ret);
        return ADF4382A_MANAGER_ERROR_SPI;
    }
    
    // Small delay for sync to take effect
    no_os_udelay(10);

    // Clear software sync
    ret = adf4382_set_sw_sync(manager->tx_dev, false);
    if (ret) {
        printf("TX sync clear failed: %d\n", ret);
        return ADF4382A_MANAGER_ERROR_SPI;
    }
    
    ret = adf4382_set_sw_sync(manager->rx_dev, false);
    if (ret) {
        printf("RX sync clear failed: %d\n", ret);
        return ADF4382A_MANAGER_ERROR_SPI;
    }

    printf("EZSync triggered via SPI\n");
    return ADF4382A_MANAGER_OK;
}

int ADF4382A_Manager_Deinit(ADF4382A_Manager *manager)
{
    if (!manager || !manager->initialized) {
        return ADF4382A_MANAGER_ERROR_NOT_INIT;
    }

    // Disable outputs first
    if (manager->tx_dev) {
        adf4382_set_en_chan(manager->tx_dev, 0, false);
        adf4382_set_en_chan(manager->tx_dev, 1, false);
    }

    if (manager->rx_dev) {
        adf4382_set_en_chan(manager->rx_dev, 0, false);
        adf4382_set_en_chan(manager->rx_dev, 1, false);
    }

    // Remove devices
    if (manager->tx_dev) {
        adf4382_remove(manager->tx_dev);
        manager->tx_dev = NULL;
    }

    if (manager->rx_dev) {
        adf4382_remove(manager->rx_dev);
        manager->rx_dev = NULL;
    }
    
    // Disable chips and phase control pins
    set_chip_enable(TX_CE_Pin, false);
    set_chip_enable(RX_CE_Pin, false);
    set_deladj_pin(0, false);
    set_deladj_pin(1, false);
    set_delstr_pin(0, false);
    set_delstr_pin(1, false);

    manager->initialized = false;

    printf("ADF4382A Manager deinitialized\n");
    return ADF4382A_MANAGER_OK;
}

int ADF4382A_CheckLockStatus(ADF4382A_Manager *manager, bool *tx_locked, bool *rx_locked)
{
    uint8_t tx_status, rx_status;
    int ret;
    
    if (!manager || !manager->initialized || !tx_locked || !rx_locked) {
        return ADF4382A_MANAGER_ERROR_NOT_INIT;
    }
    
    // Read lock status from registers
    ret = adf4382_spi_read(manager->tx_dev, 0x58, &tx_status);
    if (ret) {
        printf("TX lock status read failed: %d\n", ret);
        return ADF4382A_MANAGER_ERROR_SPI;
    }
    
    ret = adf4382_spi_read(manager->rx_dev, 0x58, &rx_status);
    if (ret) {
        printf("RX lock status read failed: %d\n", ret);
        return ADF4382A_MANAGER_ERROR_SPI;
    }
    
    *tx_locked = (tx_status & ADF4382_LOCKED_MSK) != 0;
    *rx_locked = (rx_status & ADF4382_LOCKED_MSK) != 0;
    
    // Also check GPIO lock detect pins as backup
    bool tx_gpio_locked = HAL_GPIO_ReadPin(TX_LKDET_GPIO_Port, TX_LKDET_Pin) == GPIO_PIN_SET;
    bool rx_gpio_locked = HAL_GPIO_ReadPin(RX_LKDET_GPIO_Port, RX_LKDET_Pin) == GPIO_PIN_SET;
    
    // Use both register and GPIO status
    *tx_locked = *tx_locked && tx_gpio_locked;
    *rx_locked = *rx_locked && rx_gpio_locked;
    
    return ADF4382A_MANAGER_OK;
}

int ADF4382A_SetOutputPower(ADF4382A_Manager *manager, uint8_t tx_power, uint8_t rx_power)
{
    int ret;
    
    if (!manager || !manager->initialized) {
        return ADF4382A_MANAGER_ERROR_NOT_INIT;
    }
    
    // Clamp power values (0-15)
    tx_power = (tx_power > 15) ? 15 : tx_power;
    rx_power = (rx_power > 15) ? 15 : rx_power;
    
    // Set TX power for both channels
    ret = adf4382_set_out_power(manager->tx_dev, 0, tx_power);
    if (ret) return ret;
    ret = adf4382_set_out_power(manager->tx_dev, 1, tx_power);
    if (ret) return ret;
    
    // Set RX power for both channels
    ret = adf4382_set_out_power(manager->rx_dev, 0, rx_power);
    if (ret) return ret;
    ret = adf4382_set_out_power(manager->rx_dev, 1, rx_power);
    
    printf("Output power set: TX=%d, RX=%d\n", tx_power, rx_power);
    return ADF4382A_MANAGER_OK;
}

int ADF4382A_EnableOutputs(ADF4382A_Manager *manager, bool tx_enable, bool rx_enable)
{
    int ret;
    
    if (!manager || !manager->initialized) {
        return ADF4382A_MANAGER_ERROR_NOT_INIT;
    }
    
    // Enable/disable TX outputs
    ret = adf4382_set_en_chan(manager->tx_dev, 0, tx_enable);
    if (ret) return ret;
    ret = adf4382_set_en_chan(manager->tx_dev, 1, tx_enable);
    if (ret) return ret;
    
    // Enable/disable RX outputs
    ret = adf4382_set_en_chan(manager->rx_dev, 0, rx_enable);
    if (ret) return ret;
    ret = adf4382_set_en_chan(manager->rx_dev, 1, rx_enable);
    
    printf("Outputs: TX=%s, RX=%s\n", 
           tx_enable ? "ENABLED" : "DISABLED",
           rx_enable ? "ENABLED" : "DISABLED");
    return ADF4382A_MANAGER_OK;
}

// New phase delay functions

int ADF4382A_SetPhaseShift(ADF4382A_Manager *manager, uint16_t tx_phase_ps, uint16_t rx_phase_ps)
{
    if (!manager || !manager->initialized) {
        return ADF4382A_MANAGER_ERROR_NOT_INIT;
    }

    // Clamp phase shift values
    tx_phase_ps = (tx_phase_ps > PHASE_SHIFT_MAX_PS) ? PHASE_SHIFT_MAX_PS : tx_phase_ps;
    rx_phase_ps = (rx_phase_ps > PHASE_SHIFT_MAX_PS) ? PHASE_SHIFT_MAX_PS : rx_phase_ps;

    // Convert phase shift to duty cycle and apply
    if (tx_phase_ps != manager->tx_phase_shift_ps) {
        uint16_t duty_cycle = phase_ps_to_duty_cycle(tx_phase_ps);
        ADF4382A_SetFinePhaseShift(manager, 0, duty_cycle); // 0 = TX device
        manager->tx_phase_shift_ps = tx_phase_ps;
    }

    if (rx_phase_ps != manager->rx_phase_shift_ps) {
        uint16_t duty_cycle = phase_ps_to_duty_cycle(rx_phase_ps);
        ADF4382A_SetFinePhaseShift(manager, 1, duty_cycle); // 1 = RX device
        manager->rx_phase_shift_ps = rx_phase_ps;
    }

    printf("Phase shift set: TX=%d ps, RX=%d ps\n", tx_phase_ps, rx_phase_ps);
    return ADF4382A_MANAGER_OK;
}

int ADF4382A_GetPhaseShift(ADF4382A_Manager *manager, uint16_t *tx_phase_ps, uint16_t *rx_phase_ps)
{
    if (!manager || !manager->initialized || !tx_phase_ps || !rx_phase_ps) {
        return ADF4382A_MANAGER_ERROR_NOT_INIT;
    }

    *tx_phase_ps = manager->tx_phase_shift_ps;
    *rx_phase_ps = manager->rx_phase_shift_ps;

    return ADF4382A_MANAGER_OK;
}

int ADF4382A_SetFinePhaseShift(ADF4382A_Manager *manager, uint8_t device, uint16_t duty_cycle)
{
    if (!manager || !manager->initialized) {
        return ADF4382A_MANAGER_ERROR_NOT_INIT;
    }

    // Clamp duty cycle
    duty_cycle = (duty_cycle > DELADJ_MAX_DUTY_CYCLE) ? DELADJ_MAX_DUTY_CYCLE : duty_cycle;

    // For simplicity, we'll use a basic implementation
    // In a real system, you would generate a PWM signal on DELADJ pin
    // Here we just set the pin state based on a simplified approach

    if (duty_cycle == 0) {
        set_deladj_pin(device, false);
    } else if (duty_cycle >= DELADJ_MAX_DUTY_CYCLE) {
        set_deladj_pin(device, true);
    } else {
        // For intermediate values, you would need PWM generation
        // This is a simplified implementation
        set_deladj_pin(device, true);
    }

    printf("Device %d DELADJ duty cycle set to %d/%d\n",
           device, duty_cycle, DELADJ_MAX_DUTY_CYCLE);

    return ADF4382A_MANAGER_OK;
}

int ADF4382A_StrobePhaseShift(ADF4382A_Manager *manager, uint8_t device)
{
    if (!manager || !manager->initialized) {
        return ADF4382A_MANAGER_ERROR_NOT_INIT;
    }

    // Generate a pulse on DELSTR pin to latch the current DELADJ value
    set_delstr_pin(device, true);
    no_os_udelay(DELADJ_PULSE_WIDTH_US);
    set_delstr_pin(device, false);

    printf("Device %d phase shift strobed\n", device);

    return ADF4382A_MANAGER_OK;
}

// Static helper functions

static void set_chip_enable(uint8_t ce_pin, bool state)
{
    GPIO_TypeDef* port = (ce_pin == TX_CE_Pin) ? TX_CE_GPIO_Port : RX_CE_GPIO_Port;
    HAL_GPIO_WritePin(port, ce_pin, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void set_deladj_pin(uint8_t device, bool state)
{
    if (device == 0) { // TX device
        HAL_GPIO_WritePin(TX_DELADJ_GPIO_Port, TX_DELADJ_Pin, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
    } else { // RX device
        HAL_GPIO_WritePin(RX_DELADJ_GPIO_Port, RX_DELADJ_Pin, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
}

static void set_delstr_pin(uint8_t device, bool state)
{
    if (device == 0) { // TX device
        HAL_GPIO_WritePin(TX_DELSTR_GPIO_Port, TX_DELSTR_Pin, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
    } else { // RX device
        HAL_GPIO_WritePin(RX_DELSTR_GPIO_Port, RX_DELSTR_Pin, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
}

static uint16_t phase_ps_to_duty_cycle(uint16_t phase_ps)
{
    // Convert phase shift in picoseconds to DELADJ duty cycle
    // This is a linear mapping - adjust based on your specific requirements
    uint32_t duty = (uint32_t)phase_ps * DELADJ_MAX_DUTY_CYCLE / PHASE_SHIFT_MAX_PS;
    return (uint16_t)duty;
}
