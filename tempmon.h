int initTempMon();
int8_t init_sensor_data_normal_mode(struct bme280_dev *dev);
int8_t stream_sensor_data_normal_mode(struct bme280_dev *dev,struct bme280_data *comp_data );
int8_t stream_sensor_data_forced_mode(struct bme280_dev *dev,struct bme280_data *comp_data);


