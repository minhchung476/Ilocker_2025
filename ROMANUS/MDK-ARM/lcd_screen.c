#include "main.h"
#include "i2c_lcd.h"


void printDemoWelcome(void)
{
	lcd_clear(&lcd1);
	lcd_print(0, "STM32 I2C LCD");
	lcd_print(1, "Library Demo");
}

void printRootMenu(void)
{
	lcd_clear(&lcd1);
	lcd_print(0, "1.GUI DO");
	lcd_print(1, "2.LAY DO");
}

void printInvalidOption(void)     //khi nhan nut khong hop le
{
	lcd_clear(&lcd1);
	lcd_print(0, "Invalid option");
	lcd_print(1, "Please again!");
}

void printNoLockerAvailable(void)  //khong còn locker free
{
	lcd_clear(&lcd1);
	lcd_print(0, "Not found!");
	lcd_print(1, "Try again!");
}

void printUserMenu(void)
{
	lcd_clear(&lcd1);
	lcd_print(0, "1. Check Locker Status");
	lcd_print(1, "2. Select a Locker");
}

void printUserMenu1(void)
{
	lcd_clear(&lcd1);
	lcd_print(0, "1. Check Locker Status");
	lcd_print(1, "2. Choose your Locker");
}

void printSelectLockerSize(void)
{
	lcd_clear(&lcd1);
	lcd_print(0, "1. Small Locker");
	lcd_print(1, "2. Big Locker");
}

void printLockerOpenned(struct Locker *mLocker) //hien thi len LCD 1 locker dang mo va yeu cau nguoi dung thiet lap mat khau
{
	lcd_clear(&lcd1);
	char outStr[16];
	snprintf(outStr, 16, "L%d[%c] open, hit",mLocker->lockerID, mLocker->size == SMALL?'S':'B');  //xem xet
	lcd_print(0, outStr);
	lcd_print(1, "a key to set pw");
}
void printLockerClosed(struct Locker *mLocker)  //hien thi thong tin locker dóng thông báo tam biet
{
	lcd_clear(&lcd1);	
	char outStr[16];
	snprintf(outStr, 16, "L%d[%c] closed",mLocker->lockerID, mLocker->size == SMALL?'S':'B');  //xem xet
	lcd_print(0, outStr);
	lcd_print(1, "Goodbye...");
}
void printPasswordNotMatch()  //mat khau khong khop
{
	lcd_clear(&lcd1);
	lcd_print(0, "PW not matched");
}

void printPasswordSetSuccess()   //mat khau phu hop
{
	lcd_clear(&lcd1);
	lcd_print(0, "PW is set");
	lcd_print(1, "Sucessful");
}

void printReleaseChoice()  //lua chon tiep tuc su dung hay tra lai tu
{
	lcd_clear(&lcd1);
	lcd_print(0, "Welcome back!");
	HAL_Delay(2000);
	lcd_clear(&lcd1);
	lcd_print(0, "1.Keep using");
	lcd_print(1, "2.Release locker");
}

void printEnjoy(void)   //thong bao nguoi su dung hoan tat viec su dung locker, yeu cau nhan phim bat ky de dóng
{
	lcd_clear(&lcd1);
	lcd_print(0, "Enjoy your");
	lcd_print(1, "locker!");
	HAL_Delay(2000);
	lcd_clear(&lcd1);
	lcd_print(0, "Done? Press Any");
	lcd_print(1, "key to close");
}