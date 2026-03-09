#include "stm32_spi.h"
#include "no_os_error.h"
#include <stdlib.h>

int32_t stm32_spi_init(struct no_os_spi_desc **desc,
                       const struct no_os_spi_init_param *param)
{
    if (!desc || !param)
        return -EINVAL;

    *desc = calloc(1, sizeof(**desc));
    if (!*desc)
        return -ENOMEM;

    /* store platform handle (HAL SPI_HandleTypeDef*) in extra */
    (*desc)->extra = param->extra;
    (*desc)->max_speed_hz = param->max_speed_hz;
    (*desc)->mode = param->mode;

    return 0;
}

int32_t stm32_spi_write_and_read(struct no_os_spi_desc *desc,
                                 uint8_t *data,
                                 uint32_t bytes_number)
{
    if (!desc || !data || bytes_number == 0)
        return -EINVAL;

    SPI_HandleTypeDef *hspi = (SPI_HandleTypeDef *)desc->extra;
    if (!hspi)
        return -EINVAL;

    if (HAL_SPI_TransmitReceive(hspi, data, data, bytes_number, 200) != HAL_OK)
        return -EIO;

    return 0;
}

int32_t stm32_spi_remove(struct no_os_spi_desc *desc)
{
    if (!desc)
        return -EINVAL;
    free(desc);
    return 0;
}

/* platform ops struct */
const struct no_os_spi_platform_ops stm32_spi_ops = {
    .init = &stm32_spi_init,
    .write_and_read = &stm32_spi_write_and_read,
    .remove = &stm32_spi_remove,
};
