# A ROS node converting MAVROS to mav_msgs

## Contributors


---

## License

Licensed under the Boost Software License 1.0, see LICENSE file for details.

---

## Functionality

Converts the main command and sensor topics to mav_msgs format. Still WIP.


## Dependencies

1.
sudo apt install geographiclib-tools

wget https://raw.githubusercontent.com/mavlink/mavros/master/mavros/scripts/install_geographiclib_datasets.sh

sudo chmod +x ./install_geographiclib_datasets.sh

sudo ./install_geographiclib_datasets.sh

## Permisions

sudo chmod 666 /dev/ttyACM0

sudo gpasswd -a $USER dialout


