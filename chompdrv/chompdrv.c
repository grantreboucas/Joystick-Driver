#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <linux/uinput.h>
#include <libusb-1.0/libusb.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <linux/input.h>
#include <linux/joystick.h>
//gonna need to conver decimal to binary when reading for values from joystick. Talked about in report
void toBin(int num, char *dec)
{
	*(dec + 5) = '\0';
	int temp = 0x10 << 1;
	while(temp >>= 1)
	{
		*dec++ = !!(temp & num) + '0';
	}
}


//gotten from the uinput tutorial link from P4 documentation
void emit(int fd, int type, int code, int val)
{
	struct input_event js;
	js.type = type;
	js.code = code;
	js.value = val;
	js.time.tv_sec = 0;
	js.time.tv_usec = 0;
	write(fd, &js, sizeof(js));
}

int main(void)
{
	//things need to set up structs for uinput and libsusb. Currently based on the guides in the P4 documentation
	libusb_device_handle *dev_handle;
	libusb_context *ctx = NULL;
	int r;
	int error;
	struct uinput_setup  usetup;

	int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
	if(fd < 0)
	{
		printf("Input device not found\n");
		return 0;
	}

	//Setting up keys to monitor. Not sure if you can use KEY_UP etc. But we care about the axis of x and y and the "BTN_JOYSTICK" which is just spacebar
	error = ioctl(fd, UI_SET_EVBIT, EV_KEY);
	if(error < 0)
	{
		printf("EV_KEY error\n");
		return 0;
	}
	error =ioctl(fd, UI_SET_EVBIT, EV_SYN);
	if(error < 0)
	{
		printf("EV_SYN error\n");
		return 0;
	}
        ioctl(fd, UI_SET_KEYBIT, BTN_JOYSTICK);
	ioctl(fd, UI_SET_EVBIT, EV_ABS);
        ioctl(fd, UI_SET_ABSBIT, ABS_X);
        ioctl(fd, UI_SET_EVBIT, EV_ABS);
        ioctl(fd, UI_SET_ABSBIT, ABS_Y);

	//based off of code in uinput guide
	memset(&usetup, 0, sizeof(usetup));
	usetup.id.bustype = BUS_USB;

	//Not sure if you could have converted the hex to binary had have it work, but I was having problems when using the binary version of it
	usetup.id.vendor = 0x9A7A;
	usetup.id.product = 0xBA17;
	usetup.id.version = 1;
	strcpy(usetup.name, "Chompstick");
	write(fd, &fd, sizeof(fd));

	ioctl(fd, UI_DEV_SETUP, &usetup);
	ioctl(fd, UI_DEV_CREATE);

	sleep(1);

	//libsusb guide
	r = libusb_init(&ctx);
	if(r <0)
	{
		printf("Error with Init\n");
		return 1;
	}
	libusb_set_debug(ctx,3);

	dev_handle = libusb_open_device_with_vid_pid(ctx, 0x9A7A, 0xBA17);
	if(dev_handle == NULL)
	{
		printf("Cannot open device\n");
		return 1;
	}

	//not sure how this is going to work. Going to base of assumption of 1 device being the chompapp
	r = libusb_claim_interface(dev_handle, 0);
	if(r < 0)
	{
		printf("Cannot claim interface\n");
		return 1;
	}

	bool yes = true;
	//gonna reflect how many bytes were writted, P4 talks about sending a single byte of data. libusb guide.
	int actual;
	//libsub guide. get get what data we're writing in a while loop, so dont set it to anything at the moment
	unsigned char data[1];
	//jtest works by setting values equal to certain numbers based the status of the button, x-axis, and y-axis, so we make ints t
	int button;
	int x_axis;
	int y_axis;

	//while loop to keep reading from joystick until the user hits q
	while(yes)
	{
		data[0] = '\0';
		//based off of libusb guide. 129 is the endpoint gotten from listing info about all the open usb devices including chompapp
		r = libusb_interrupt_transfer(dev_handle, 129, data, sizeof(data), &actual, 0);
		//made in while loop to reset everytime a new button that isnt q is pressed. have to convert the decimal in data to binary
		char input[4];
		toBin((int)data[0], input);

		//now we can start our if statements to read the inputs. based off P4 doc, 1st one is button, next 2 are x axis, then last 2 are y axis
		if(input[0] == '1')
		{
			//button is on
			button = 1;
			emit(fd, EV_KEY, BTN_JOYSTICK, 1);
			emit(fd, EV_SYN, SYN_REPORT, 0);
		}
		else if(input[0] =='0')
		{
			//button is off
			button = 0;
			emit(fd, EV_KEY, BTN_JOYSTICK, 0);
			emit(fd, EV_SYN, SYN_REPORT, 0);
		}
		if(input[1] == '0' && input[2] == '1')
		{
			//xaxis is 1
			x_axis = -32767;
			emit(fd, EV_ABS, ABS_X, x_axis);
                        emit(fd, EV_SYN, SYN_REPORT, 0);
		}
		if(input[1] == '1' && input[2] == '0')
		{
			//xaxis is 2
			x_axis = 0;
			emit(fd, EV_ABS, ABS_X, x_axis);
                        emit(fd, EV_SYN, SYN_REPORT, 0);
		}
		else if(input[1] == '1' && input[2] == '1')
		{
			//xaxis is 3
			x_axis = 32767;
			emit(fd, EV_ABS, ABS_X, x_axis);
                        emit(fd, EV_SYN, SYN_REPORT, 0);
		}
		if(input[3] == '0' && input[4] == '1')
		{
			//yaxis is 1
			y_axis = 32767;
			emit(fd, EV_ABS, ABS_Y, y_axis);
                        emit(fd, EV_SYN, SYN_REPORT, 0);
		}
		if(input[3] == '1' && input[4] == '0')
		{
			//yaxis is 2
			y_axis = 0;
			emit(fd, EV_ABS, ABS_Y, y_axis);
                        emit(fd, EV_SYN, SYN_REPORT, 0);
		}
		else if(input[3] == '1' && input[4] == '1')
		{
			//yaxis is 3
			y_axis = -32767;
			emit(fd, EV_ABS, ABS_Y, y_axis);
                        emit(fd, EV_SYN, SYN_REPORT, 0);
		}

	}
	//while loop is exited when user presses q
	//libusb guide
	r = libusb_release_interface(dev_handle, 0);
	libusb_close(dev_handle);
	libusb_exit(ctx);
	//uinput guide
	ioctl(fd, UI_DEV_DESTROY);
	close(fd);

	return 0;
}

