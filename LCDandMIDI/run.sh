echo "Disabling all bridges"
echo 0 > /sys/class/fpga-bridge/fpga2hps/enable
echo 0 > /sys/class/fpga-bridge/hps2fpga/enable
echo 0 > /sys/class/fpga-bridge/lwhps2fpga/enable

echo "\nEnabling Lightweight HPS-FPGA bridge"
echo 1 > /sys/class/fpga-bridge/lwhps2fpga/enable

echo "Running program"
./bridge

echo "Program Complete. Disabling bridge"
echo 0 > /sys/class/fpga-bridge/lwhps2fpga/enable