/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "i2c_lcd.h"
#include "keypad_4x4.h"
#include "stdio.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum{
	SMALL,
	BIG,
}lockerSize;

typedef enum{
	OPENED,
	LOCKED
}lockerState;

enum{
	ROOT_MENU,
	OPTION_1,
	OPTION_2,
	INVALID_KEY
};

typedef enum {
    OPEN_LOCKER,
    CLOSE_LOCKER,
    PRINT_OPENED_NOTICE,  //thong bao locker mo
    PASSWORD_AUTHEN,      //xac thuc mat khau co khop voi mat khau da luu hay không
    SET_NEW_PASSWORD,     //thiet lap mat khau moi
    WAIT_USER,            //cho hanh dong tu nguoi dung
    RELEASE_LOCKER,       //giai phong lock sau khi su dung
    FORCE_OPEN_LOCKER,    //mo locker khi khan cap
    HANDLE_LOCKER,        //su dung 
    EXIT_USER_APP         //thoat khoi ung dung nguoi dung
} LockerHandlerState;

typedef enum {               //trang thai hoat dong cua he thong danh cho nguoi su dung
    PRINT_USER_MENU,         //trang thai mac dinh
    PRINT_LOCKERS_STATUS,    //hien thi trang thai tu
    SELECT_A_LOCKER,         //chon tu 
    INVALID_USER_OPTION,     //khong hop le
    SET_LOCKER_PASSWORD,     //thiet lap mat khau
    EXIT_USER_MODE           //thoat khoi trang thai nguoi su dung
} UserState;

typedef enum {                //trang thai hoat dong cua he thong danh cho nguoi su dung
    PRINT_USER_MENU1,         //trang thai mac dinh
    PRINT_LOCKERS_STATUS1,    //hien thi trang thai tu
    SELECT_A_LOCKER1,         //chon tu 
    INVALID_USER_OPTION1,     //khong hop le
    SET_LOCKER_PASSWORD1,     //thiet lap mat khau
    EXIT_USER_MODE1           //thoat khoi trang thai nguoi su dung
} UserState1;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define PASS_WORD_LENGTH 4
#define NUMBER_OF_LOCKERS 4
#define LOCKER_GPIO_PORT GPIOB
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

I2C_HandleTypeDef hi2c1;
I2C_LCD_HandleTypeDef lcd1;
UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
uint8_t key = 0;
char Rx_data[11]={0};
uint8_t flag_interrupt=0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
struct Locker
{
	int lockerID;
	int userID;
	bool isOcupied;
	lockerSize size;
	lockerState state;
	bool isPasswordSet;
	uint16_t io;
	uint8_t password[PASS_WORD_LENGTH];
	uint8_t otp[PASS_WORD_LENGTH];
};

struct  Locker Lockers[NUMBER_OF_LOCKERS];

void initLocker(struct  Locker *Locker, int lockerID, lockerSize size, uint16_t GPIOPin)
{
	Locker->lockerID = lockerID;
	Locker->isOcupied = false;
	Locker->size = size;
	Locker->state = LOCKED;           //closed
	Locker->isPasswordSet = false;
	Locker->userID=-1;
	Locker->io = GPIOPin;
}	

void openLocker(struct  Locker *Locker)
{
	HAL_GPIO_WritePin(LOCKER_GPIO_PORT, Locker->io, GPIO_PIN_SET);
}

void closeLocker(struct  Locker *Locker)
{
	HAL_GPIO_WritePin(LOCKER_GPIO_PORT, Locker->io, GPIO_PIN_RESET);
}

void setNumberOTP(struct Locker *Locker) {
    if (Locker == NULL) {
        return;
    }
    int a = 1000, b = 9999;
    int number = rand() % (b - a + 1) + a; 
    memset(Locker->otp, 0, PASS_WORD_LENGTH); 
    snprintf((char *)Locker->otp, 5, "%d", number); 
		return;
}

void setPassword(struct  Locker *Locker, uint8_t *pw, int pwLen)
{
	if(pw == NULL || pwLen != PASS_WORD_LENGTH)
	{
		return; 
	}
	Locker->isPasswordSet = 1;
	memcpy(Locker->password, pw, PASS_WORD_LENGTH);
	return;
}
/**
 * @brief return 0 if password matched
 * 
 * @param Locker 
 * @param pw 
 * @param pwLen 
 * @return int 
 */
int validateLockerAccess(struct Locker *Locker, uint8_t *input, int length) {
    if (!Locker->isOcupied || input == NULL || length != PASS_WORD_LENGTH) {
        return 0;  
    }
    if (memcmp(Locker->password, input, PASS_WORD_LENGTH) == 0 ||
        memcmp(Locker->otp, input, PASS_WORD_LENGTH) == 0) {
        return 1;  
    }
    return 0;  
}

void changePassword(struct  Locker *Locker, uint8_t *pw, int pwLen)
{

}

struct Locker *getNewLocker(lockerSize size)
{
	for(int i = 0; i < NUMBER_OF_LOCKERS; i++)
	{
		if(!Lockers[i].isOcupied && !Lockers[i].isPasswordSet && Lockers[i].size == size)   //chua duoc su dung, co kich thuoc dung
		{
			return &Lockers[i];
		}
	}
	return NULL;
}

void assignLockerToUser(int userID, struct Locker *locker)  //gan locker cho nguoi dung
{
	locker->userID = userID;//chi dinh nguoi dung
	locker->isOcupied = true;  //da duoc su dung
	locker->state = OPENED;
	locker->isPasswordSet = true; //ky la*****
}

void releaseLocker(struct Locker *locker)   //giai phong locker, ve trang thai chua co nguoi dung
{
	locker->userID = 0;
	locker->isOcupied = false;
	locker->state = OPENED;
	locker->isPasswordSet = false;
}
void lcd_print(int line, char *str)
{
	if(str == NULL || (line != 0 && line != 1))
		return;
	lcd_gotoxy(&lcd1, 0, line);
	lcd_puts(&lcd1, str);
}

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
void printLockerStatus(void)
{
	char outStr[16];
	lcd_clear(&lcd1);
	int line = 0;
	int offset = 0; //dieu chinh vi tri bat dau in

	for(int i = 0; i < NUMBER_OF_LOCKERS; i++)
	{
		snprintf(outStr, 16, "L%d[%c] %s", Lockers[i].lockerID, \
		Lockers[i].size == SMALL?'S':'B', Lockers[i].isOcupied?"BUSY":"FREE");
    
		lcd_gotoxy(&lcd1, offset, line);
		lcd_puts(&lcd1, outStr);
		offset += 8;
		if(offset/16){
			line +=1;
			offset = 0;
		}
	}
}

bool isFreeLocker(struct Locker *mLocker)
{
	return !mLocker->isOcupied;       
}

void UART_SendString(UART_HandleTypeDef *huart, char *str){
    if(flag_interrupt == 0) {  
        flag_interrupt = 1;
        HAL_UART_Transmit_IT(huart, (uint8_t*) str, strlen(str));
    }
}


void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        flag_interrupt = 0;  
    }
}

//selectedLocker = key -'0' ;
//Lockers[selectedLocker].lockerID = key ; 
//mLocker->lockerID = Lockers[selectedLocker].lockerID;
struct Locker *getYourLocker(int yourLockerID){                  
    lcd_clear(&lcd1);
    lcd_print(0, "Choose your locker:");
    uint8_t key = 0;
    int count = 0; 
    uint8_t selectedLocker = NULL; 
	  char lockerStr[2];
    while (1) { 
        key = keypad_get_key_value();
 
        if (key == '0' || key == '1' || key == '2' || key == '3') { 
            selectedLocker = key - '0'; 
					  snprintf(lockerStr, sizeof(lockerStr), "%d", selectedLocker);
            lcd_gotoxy(&lcd1, count++, 1);
            lcd_puts(&lcd1,lockerStr);
        } else if (key == '#') { 
            if (count > 0) break; 
        }
				else{
					lcd_clear(&lcd1);
          lcd_print(0, "Invalid key!");
          HAL_Delay(1000); 
          lcd_clear(&lcd1);
          lcd_print(0, "Choose your locker:");
				}
        HAL_Delay(200); 
    }
		
    yourLockerID = selectedLocker;
    lcd_clear(&lcd1);
    lcd_print(0, "Locker Selected:");
    char outStr[16];
    snprintf(outStr, 16, "Locker %d", yourLockerID);
    lcd_gotoxy(&lcd1, 0, 1);
    lcd_print(1, outStr);
    return &Lockers[yourLockerID];
}

void getPasswordForKeyPass(uint8_t *pw)
{
	lcd_clear(&lcd1);
	lcd_print(0, "PW-4digit"); // Enter 4 last digits, and hit Enter.
	int count = 0;
	uint8_t key;
	key = keypad_get_key_value();
	
	while(key != '#' || count < PASS_WORD_LENGTH)
	{
		if(count < PASS_WORD_LENGTH){
			pw[count]= key;
			lcd_gotoxy(&lcd1, count++, 1);
			lcd_puts(&lcd1, "*");
		}else{
			for(int i = 0; i < PASS_WORD_LENGTH - 1; i++)      //khi vuot quá do dài mat khau, dich mang và thêm ký tu moi	
			{
				pw[i] = pw[i+1];
			}
			pw[PASS_WORD_LENGTH - 1]= key - '0';
		}
		key = keypad_get_key_value();
	}
	return;
}

void getVerifyPassword(uint8_t *pw)
{
	lcd_clear(&lcd1);
	lcd_print(0, "Verify PW"); // Enter 4 last digits, and hit Enter.
	int count = 0;
	uint8_t key;
	key = keypad_get_key_value();
	while(key != '#' || count < PASS_WORD_LENGTH)
	{
		if(count < PASS_WORD_LENGTH){
			pw[count]= key;
			lcd_gotoxy(&lcd1, count++, 1);
			lcd_puts(&lcd1, "*");
		}else{
			for(int i = 0; i < PASS_WORD_LENGTH - 1; i++)
			{
				pw[i] = pw[i+1];
 			}
			pw[PASS_WORD_LENGTH - 1]= key - '0';
		}
		key = keypad_get_key_value();
	}
	return;
}

int lockerHandler(struct Locker *mLocker)
{
	int _state = OPEN_LOCKER;
	uint8_t pw[PASS_WORD_LENGTH];    //luu tru mat khau
	uint8_t confirmPW[PASS_WORD_LENGTH];   //mat khau xac nhan
	memset(pw, 0, PASS_WORD_LENGTH);
	memset(confirmPW, 0, PASS_WORD_LENGTH);
	uint8_t key;
	while(1)
	{
		switch(_state){
			case OPEN_LOCKER:
				if(isFreeLocker(mLocker))   //kiem tra locker dang ranh hay không
				{
					// do open.
					openLocker(mLocker);   //open locker
					_state = PRINT_OPENED_NOTICE;   //thong bao
				}
				else{
					_state = PASSWORD_AUTHEN;   //tu da duoc dung. xac nhan xem mat khau co dung không
				}
				break;
			case FORCE_OPEN_LOCKER:  //mo khân câp
				// do open here
				openLocker(mLocker);
			  setNumberOTP(mLocker);
			  char msg[30];
			  sprintf(msg,"L%d is Opened. New OTP:%s\r\n",mLocker->lockerID, mLocker->otp);	
        UART_SendString(&huart1, msg);
				printReleaseChoice();
				key = keypad_get_key_value();
				if(key == '1')
				{
					_state = WAIT_USER;      //tiep tuc su dung
				}else if(key == '2')
				{
					_state = RELEASE_LOCKER;  //tra lai tu
				}
				break;
			case PRINT_OPENED_NOTICE:   //hien thi thong bao tu duoc mo,doi nguoi dung nhan phim xac nhan
				printLockerOpenned(mLocker);
				keypad_get_key_value();//....
				_state = SET_NEW_PASSWORD;
				break;
			case PASSWORD_AUTHEN:         //xac thuc mat khau
			 	getPasswordForKeyPass(pw);  //lay mat khau nguoi dung thông qua keypad		
			if(validateLockerAccess(mLocker, pw, PASS_WORD_LENGTH)==1)   //kiem tra mat khau nhap vao
				{
					// match
					_state = FORCE_OPEN_LOCKER;  //mo tu
				}else{
					printPasswordNotMatch();
					HAL_Delay(2000);
					_state = EXIT_USER_APP; //thoat thao tac
				}
				break;
			
			case SET_NEW_PASSWORD:   //nhap pass moi
				getPasswordForKeyPass(pw);
				getVerifyPassword(confirmPW);
				if(memcmp(pw, confirmPW, PASS_WORD_LENGTH) == 0)
				{
					// matched:
					setPassword(mLocker, pw, PASS_WORD_LENGTH);
					printPasswordSetSuccess();
					HAL_Delay(2000);
					_state = WAIT_USER;
				}else{
					printPasswordNotMatch(); // set new password again
					HAL_Delay(2000);
				}
				break;
			case WAIT_USER:
				printEnjoy();
				assignLockerToUser(mLocker->lockerID, mLocker);          //xac nhan dang su dung tu hoan tat
				keypad_get_key_value();
				_state = CLOSE_LOCKER;
			  break;
			case CLOSE_LOCKER:
				// Do close here;
				closeLocker(mLocker);
			  mLocker->state = LOCKED;
				printLockerClosed(mLocker);
			  HAL_Delay(3000);
				_state = EXIT_USER_APP;
				break;
			case RELEASE_LOCKER:
				releaseLocker(mLocker);
			  closeLocker(mLocker);
				printLockerClosed(mLocker);
				_state = EXIT_USER_APP;
				break;
			case EXIT_USER_APP:
				return 0;
				break;		
			default:
				break;
		}
		//if(_state == EXIT_USER_APP) break;
	}
}

int leaveBelongingsHandler(void)
{
	uint8_t pw[PASS_WORD_LENGTH];
	int userState = PRINT_USER_MENU;              //hien thi menu cho nguoi dùng
	uint8_t key = 0;
	struct Locker *mLocker = NULL;
	uint8_t confirmPW[PASS_WORD_LENGTH];
  while(1){
	switch (userState)
	{
	case PRINT_USER_MENU:
		printUserMenu();
		key = keypad_get_key_value();
		if(key == '1')
		{
			userState = PRINT_LOCKERS_STATUS;          //hien thi trang thai tu
		}else if(key == '2')
		{
			userState = SELECT_A_LOCKER;               //chon tu
		}else{
			userState = INVALID_USER_OPTION;           //hien thi nhan nut khong hop le
		}
		break;
	case PRINT_LOCKERS_STATUS:
		// Print lockers status here
		printLockerStatus();
		key = keypad_get_key_value();                 // Press any key to return to main menu;
		userState = PRINT_USER_MENU;
		break;
	case SELECT_A_LOCKER:
		printSelectLockerSize();
		key = keypad_get_key_value(); 
		if(key == '1')                                    //chon tu nho
		{
			mLocker = getNewLocker(SMALL);
		}else if(key == '2')                            //chon tu to
		{
			mLocker = getNewLocker(BIG);
		}else{
			userState = INVALID_USER_OPTION;                           //lua chon không hop le
			break;
		}
		if(mLocker == NULL)                               //không còn locker
		{
			printNoLockerAvailable();
			HAL_Delay(2000);
			userState = PRINT_USER_MENU;
		}else{
			userState = HANDLE_LOCKER;
		}
		break;
	case HANDLE_LOCKER:
		//lockerHandler(mLocker);  
	  if (lockerHandler(mLocker) == 0)
       {
        userState = EXIT_USER_MODE;
				break;
       }
		else{lockerHandler(mLocker); }
		break;
	case INVALID_USER_OPTION:
		printInvalidOption();
		userState = EXIT_USER_MODE;                       //quay lai menu sau 2s
		HAL_Delay(2000);
		break;
	case SET_LOCKER_PASSWORD:
		setPassword(mLocker, pw, PASS_WORD_LENGTH);
		break;
	case EXIT_USER_MODE:
		return 0;
		break; 
	default:
		break;
	}
	//if(userState == EXIT_USER_MODE) break; 
  }
	//return;
}

int receiveHandler(void){
	uint8_t confirmPW[PASS_WORD_LENGTH];
	struct Locker *mLocker = NULL;
	uint8_t key = 0;
	int _userState1 = PRINT_USER_MENU1;
	int YourID;
	int LockerID;
	while(1){
		switch(_userState1){
			case PRINT_USER_MENU1:
				printUserMenu1();
			  key = keypad_get_key_value();
			  if(key=='1'){
					_userState1 = PRINT_LOCKERS_STATUS1;
				}
				else if(key == '2'){
					_userState1 = SELECT_A_LOCKER1;
				}
				else{
					_userState1 = INVALID_USER_OPTION1;
				}
				break;
			case PRINT_LOCKERS_STATUS1:
				printLockerStatus();
			  key = keypad_get_key_value();
			  _userState1 = PRINT_USER_MENU1;
			  break;
			case SELECT_A_LOCKER1:
				//getYourLocker(LockerID);
			  mLocker = getYourLocker(LockerID);
	    	if(mLocker == NULL)                               //không còn locker
	    	{
	    		printNoLockerAvailable();
	    		HAL_Delay(2000);
	    		_userState1  = PRINT_USER_MENU1;
	    	}else{
	    		if(isFreeLocker(mLocker))   //kiem tra locker dang ranh hay không
				{
					_userState1 = INVALID_USER_OPTION1;   //thong bao
				}
				else{
					_userState1 = HANDLE_LOCKER;
				  }
	    	}
			  break;
			case HANDLE_LOCKER:
				if (lockerHandler(mLocker) == 0)
       {
        _userState1 = EXIT_USER_MODE1;
				break;
       }
		    else{lockerHandler(mLocker); }
			case INVALID_USER_OPTION1:
				printInvalidOption();
	    	_userState1 = EXIT_USER_MODE1;                       //quay lai menu sau 2s
		    HAL_Delay(2000);
		    break;
			case EXIT_USER_MODE1:
		    return 0;
		    break; 
			default:
				break;
		}
	}
	return 0;
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
  int mState = ROOT_MENU;
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
	
  lcd1.hi2c = &hi2c1;
	lcd1.address = 0x4E;
	lcd_init(&lcd1);
	initLocker(&Lockers[0], 0, SMALL, GPIO_PIN_0);
	initLocker(&Lockers[1], 1, SMALL, GPIO_PIN_1);
	initLocker(&Lockers[2], 2, BIG, GPIO_PIN_10);
	initLocker(&Lockers[3], 3, BIG, GPIO_PIN_11);

	printDemoWelcome();
	HAL_Delay(3000);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
		switch (mState)
		{
		case ROOT_MENU:
			printRootMenu();
			key = keypad_get_key_value();
			if(key == '1')
			{
				mState = OPTION_1;
			}else if(key == '2')
			{
				mState = OPTION_2;
			}else{
				mState = INVALID_KEY;
			}
			break;
		case OPTION_1:
			leaveBelongingsHandler();
		  if(leaveBelongingsHandler() == 0){
			mState = ROOT_MENU;}
			break;
		
		case OPTION_2:
			receiveHandler();
			HAL_Delay(2000);
			mState = ROOT_MENU;
			break;
		case INVALID_KEY:	
			printInvalidOption();
			HAL_Delay(2000);
			mState = ROOT_MENU;
			break;
		default:
			break;
		}

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV2;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL15;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, R1_Pin|R2_Pin|R3_Pin|R4_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_10|GPIO_PIN_11, GPIO_PIN_RESET);

  /*Configure GPIO pins : R1_Pin R2_Pin R3_Pin R4_Pin */
  GPIO_InitStruct.Pin = R1_Pin|R2_Pin|R3_Pin|R4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : C1_Pin C2_Pin C3_Pin C4_Pin */
  GPIO_InitStruct.Pin = C1_Pin|C2_Pin|C3_Pin|C4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB0 PB1 PB10 PB11 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_10|GPIO_PIN_11;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
