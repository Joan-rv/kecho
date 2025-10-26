# kecho
Build and test:
```sh
make
sudo insmod build/kecho.ko
sudo chmod 666 /dev/echo
exec 3<> /dev/echo
echo "test" >&3
cat <&3 # outputs "test"
exec 3>&-
sudo rmmod kecho
```
