//#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <hidsdi.h>
#include <SetupAPI.h>
#include <string>
#include <iostream>

#pragma comment (lib, "hid.lib")
#pragma comment (lib, "setupapi.lib")

#define HEXDUMP_BYTES_PER_LINE 16
void hexdump(const unsigned char* data, int length) {
	int i = 0;
	int j = 0;
	for (; i < length; i++) {
		printf(" %02X", (unsigned char)data[i]);
		if (i > 0 && (i + 1) % HEXDUMP_BYTES_PER_LINE == 0) {
			printf("\t");
			for (; j <= i; j++) {
				if (data[j] == 0x0d || data[j] == 0x0a)
					printf(".");
				else
					printf("%c", data[j]);
			}
			printf("\n");
		}
	}
	for (; (i + 1) % HEXDUMP_BYTES_PER_LINE != 0; i++) {
		printf("   ");
	}
	printf("   \t");
	for (; j < length; j++) {
		if (data[j] == 0x0d || data[j] == 0x0a || !isprint(data[j])) printf(".");
		else printf("%c", data[j]);
		//if (isprint(data[j])) printf("%c", data[j]);
		//else printf(".");
	}
	printf("\n");
}

// octal to hex
int carry10to16(int num) {
	char tmp[10];
	tmp[0] = 0;
	snprintf(tmp, sizeof(tmp), "0x%d", num);
	int ret = (int)strtol(tmp, NULL, 16);
	return ret;
}

// 04 ?? [2 bytes opcode] [4 bytes size]
// ReportID always 0x04
// Image Custom Tool 0049DFA0 is the update time function, still a lot of thing to reverse
void GMK87_UpdateTime(HANDLE h) {
	printf("Update time...\n");
	// possible tell GMK87 to start this transaction
	unsigned char data1[0x40] = {
		0x04, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};
	WriteFile(h, data1, sizeof(data1), NULL, NULL);
	// update GMK87 config here (PollingRates, Key6flag, Repeat, RepeatDelay, WFlag, WinFlag, FrameIntervalTime, LightInfo, ...
	// please check Image Custom Tool\DefaultData\keyboard.json
	unsigned char data2[64] = {
		0x04, 0x53, 0x06, 0x06, 0x30, 0x00, 0x00, 0x00, 0x00, 0x12, 0x03, 0x01, 0x00, 0x01, 0xFF, 0xFF,
		0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x02, 0xFF, 0x00, 0x00, 0x01, 0x02, 0x43, 0x17, 0x03, 0x03,
		0x04, 0x24, 0x00, 0x64, 0x00, 0x01, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};
	SYSTEMTIME ts;
	GetLocalTime(&ts);
	data2[0x31] = carry10to16(ts.wYear - 2000);
	data2[0x30] = carry10to16(ts.wMonth);
	data2[0x2f] = carry10to16(ts.wDay);
	data2[0x2e] = carry10to16(ts.wDayOfWeek);
	data2[0x2d] = carry10to16(ts.wHour);
	data2[0x2c] = carry10to16(ts.wMinute);
	data2[0x2b] = carry10to16(ts.wSecond);
	data2[0x2a] = 1; // possible WinFlag (1 -> windows, 0 -> Mac)
			 //hexdump(data2, sizeof(data2));
	WriteFile(h, data2, sizeof(data2), NULL, NULL);
	// possible tell GMK87 to finish this transaction
	unsigned char data3[64] = {
		0x04, 0x02, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};
	WriteFile(h, data3, sizeof(data3), NULL, NULL);
}

int wmain(int argc, wchar_t* argv[])
{
	// enum HID
	GUID HidGuid;
	HidD_GetHidGuid(&HidGuid);
	HDEVINFO DevInfo = SetupDiGetClassDevsW(&HidGuid, NULL, NULL, (DIGCF_PRESENT | DIGCF_DEVICEINTERFACE));
	for (int i = 0; i < 128; i++) {
		SP_DEVICE_INTERFACE_DATA DevData;
		DevData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

		BOOL ret = SetupDiEnumDeviceInterfaces(DevInfo, 0, &HidGuid, i, &DevData);
		if (ret == FALSE) continue;
		// get size first
		DWORD DevDetailSize = 0;
		ret = SetupDiGetDeviceInterfaceDetailW(DevInfo, &DevData, NULL, 0, &DevDetailSize, NULL);
		PSP_DEVICE_INTERFACE_DETAIL_DATA_W DevDetail = (PSP_DEVICE_INTERFACE_DETAIL_DATA_W)malloc(DevDetailSize);
		DevDetail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
		ret = SetupDiGetDeviceInterfaceDetailW(DevInfo, &DevData, DevDetail, DevDetailSize, &DevDetailSize, NULL);
		if (ret == FALSE) {
			free(DevDetail);
			continue;
		}

		HANDLE DevHandle = CreateFileW(DevDetail->DevicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, (LPSECURITY_ATTRIBUTES)NULL, OPEN_EXISTING, 0, NULL);
		if (DevHandle == INVALID_HANDLE_VALUE) {
			free(DevDetail);
			continue;
		}

		HIDD_ATTRIBUTES DevAttributes;
		HidD_GetAttributes(DevHandle, &DevAttributes);

		PHIDP_PREPARSED_DATA PreparsedData;
		if (HidD_GetPreparsedData(DevHandle, &PreparsedData) == FALSE) {
			CloseHandle(DevHandle);
			free(DevDetail);
			continue;
		}

		HIDP_CAPS Capabilities;
		if (HidP_GetCaps(PreparsedData, &Capabilities) == FALSE) {
			CloseHandle(DevHandle);
			free(DevDetail);
			HidD_FreePreparsedData(PreparsedData);
			continue;
		}

		//if (DevAttributes.VendorID == 0x320f && DevAttributes.ProductID == 0x5055 && Capabilities.UsagePage == 0xFFEF) {} // this is for firmware update
		if (DevAttributes.VendorID == 0x320f && DevAttributes.ProductID == 0x5055 && Capabilities.Usage == 0x92) { // 0x92 is Gaming Device Page
			printf("Found GMK87: %S\n", DevDetail->DevicePath);
			printf("  Usage = %d\n", Capabilities.Usage);
			printf("  UsagePage = %d\n", Capabilities.UsagePage);
			//hexdump((unsigned char*)&Capabilities, sizeof(Capabilities));

			GMK87_UpdateTime(DevHandle);
		}

		// dont forget to clean
		HidD_FreePreparsedData(PreparsedData);
		CloseHandle(DevHandle);
		free(DevDetail);
	}
	SetupDiDestroyDeviceInfoList(DevInfo);

	return 0;
}
