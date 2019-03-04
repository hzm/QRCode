// Fill out your copyright notice in the Description page of Project Settings.

#include "QRCodeBlueprintFunctionLibrary.h"
#include "qrencode.h"
#include <string>
#include <fstream>
#include "IImageWrapper.h"
#include "ModuleManager.h"
#include "ImageWrapper/Public/IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "FileManagerGeneric.h"

using namespace std;


#define BI_RGB	0

//#pragma pack(push, 2)必须要加。bfType是2个字节，对应“BM”，后4个字节是文件大小，又对应4字节。
//如果不设定对齐方式，而按默认的8字节或4字节对应，这些属性就错位了，而别人又是按标准来读取的，
//对齐方式不同，自然出错。一般情况下的编程之所以不考虑这些问题，是因为读取和保存都是你个人完成的，
//对齐方式是一样的，自然不出错。而这里你生成的图片可能还要供别人使用，自然要严格遵守标准。

#pragma pack(push, 2)//必须得写，否则sizeof得不到正确的结果
typedef unsigned char  BYTE;
typedef unsigned short WORD;
typedef unsigned long  DWORD;
typedef long    LONG;
typedef struct {
	WORD    bfType;//位图文件的类型，必须为BM(1-2字节）
	DWORD   bfSize;//位图文件的大小，以字节为单位（3-6字节，低位在前）
	WORD    bfReserved1;//位图文件保留字，必须为0(7-8字节）
	WORD    bfReserved2;//位图文件保留字，必须为0(9-10字节）
	DWORD   bfOffBits;//位图数据的起始位置，以相对于位图（11-14字节，低位在前）
} BITMAPFILEHEADER;

typedef struct {
	DWORD      biSize;//本结构所占用字节数（15-18字节）
	LONG       biWidth;//位图的宽度，以像素为单位（19-22字节）
	LONG       biHeight;//位图的高度，以像素为单位（23-26字节）
	WORD       biPlanes;//目标设备的级别，必须为1(27-28字节）
	WORD       biBitCount;//每个像素所需的位数，必须是1（双色）,4(16色），8(256色）16(高彩色)或24（真彩色）之一,（29-30字节）
	DWORD      biCompression;//位图压缩类型，必须是0（不压缩），1(BI_RLE8压缩类型）或2(BI_RLE4压缩类型）之一,（31-34字节）
	DWORD      biSizeImage;//位图的大小(其中包含了为了补齐行数是4的倍数而添加的空字节)，以字节为单位（35-38字节）
	LONG       biXPelsPerMeter;//位图水平分辨率，每米像素数（39-42字节）
	LONG       biYPelsPerMeter;//位图垂直分辨率，每米像素数（43-46字节)
	DWORD      biClrUsed;//位图实际使用的颜色表中的颜色数（47-50字节）
	DWORD      biClrImportant;//位图显示过程中重要的颜色数（51-54字节）
} BITMAPINFOHEADER;

#pragma pack(pop)

void UQRCodeBlueprintFunctionLibrary::GenerateQRCodeBitmap(const int32& Width, const int32& Height, const FString& Name, const FString& Outfile, int32 Margin /* = 0 */)
{
	std::string StdName(TCHAR_TO_UTF8(*Name));
	const char* QRCodeStr = StdName.c_str();
	QRcode* QRCodePtr = nullptr;
	QRCodePtr = QRcode_encodeString(QRCodeStr, 1, QR_ECLEVEL_L, QR_MODE_8, 1);
	if (QRCodePtr)
	{
		uint32 QRWidth, QRWidthAdjustedX, QRHeightAdjustedY, QRDataBytes;

		QRWidth = QRCodePtr->width;//矩阵的维数
		uint32 ScaleX = (Width - 2 * Margin) / QRWidth;
		uint32 ScaleY = (Height - 2 * Margin) / QRWidth;
		QRWidthAdjustedX = QRWidth * ScaleX;//水平维度占的像素个数（ScaleX）
		QRHeightAdjustedY = QRWidth * ScaleY;//垂直维度占的像素个数（ScaleY）
		QRDataBytes = QRWidthAdjustedX * QRHeightAdjustedY * 3;//每一个像素3个字节(BGR)

		//create data
		uint8* RGBDataPtr = (uint8 *)malloc(QRDataBytes);
		if (!RGBDataPtr)
		{
			printf("out of memory!!");
			return;
		}
		uint8* QRCodeSourceDatas = QRCodePtr->data;
		uint8* QRCodeDestData;
		memset(RGBDataPtr, 0xFF, QRDataBytes);	//分配内存，并且填充为白色

		//由于Windows规定一个扫描行所占的字节数必须是4的倍数，这里必须是4的整数倍
		int32 WidthAdjusted = Width % 4 ? (Width / 4 + 1) * 4 : Width;
		uint32 ImageBytes = WidthAdjusted * Height * 3;
		
		//create bitmap file header
		BITMAPFILEHEADER FileHeader;
		FileHeader.bfType = 0x4D42; //"BM"
		FileHeader.bfReserved1 = 0;
		FileHeader.bfReserved2 = 0;
		FileHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + ImageBytes;
		FileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

		//create bitmap info header
		BITMAPINFOHEADER InfoHeader = {0};//所有值默认为0
		InfoHeader.biSize = sizeof(BITMAPINFOHEADER);
		InfoHeader.biWidth = WidthAdjusted;
		//说明图象的高度，以象素为单位。注：这个值除了用于描述图像的高度之外，它还有另一个用处，
		//就是指明该图像是倒向的位图，还是正向的位图。如果该值是一个正数，说明图像是倒向的，
		//如果该值是一个负数，则说明图像是正向的。大多数的BMP文件都是倒向的位图，也就是时，
		//高度值是一个正数。（注：当高度值是一个负数时（正向图像），图像将不能被压缩
		//（也就是说biCompression成员将不能是BI_RLE8或BI_RLE4）。
		InfoHeader.biHeight = -(int32)Height;
		InfoHeader.biPlanes = 1;
		InfoHeader.biBitCount = 24;
		InfoHeader.biCompression = BI_RGB;
		InfoHeader.biSizeImage = ImageBytes;

		for (uint32 y = 0; y < QRWidth; y++)
		{
			QRCodeDestData = RGBDataPtr + ScaleY * y * QRWidthAdjustedX * 3;
			for (uint32 x = 0; x < QRWidth; x++)
			{
				if (*QRCodeSourceDatas & 0x01)
				{
					for (uint32 rectY = 0; rectY < ScaleY; rectY++)
					{
						for (uint32 rectX = 0; rectX < ScaleX; rectX++)
						{
							*(QRCodeDestData + rectY * QRWidthAdjustedX * 3 + rectX * 3) = 0;//Blue
							*(QRCodeDestData + rectY * QRWidthAdjustedX * 3 + rectX * 3 + 1) = 0;//Green
							*(QRCodeDestData + rectY * QRWidthAdjustedX * 3 + rectX * 3 + 2) = 0;//Red
						}
					}
				}
				QRCodeDestData += ScaleX * 3;
				QRCodeSourceDatas += 1;
			}
		}

		uint8* ImageDataPtr = (uint8 *)malloc(ImageBytes);
		memset(ImageDataPtr, 0xFF, ImageBytes);	//分配内存，并且填充为白色
		for (uint32 y = Margin; y < QRHeightAdjustedY + Margin; y++)
		{
			for (uint32 x = Margin; x < QRWidthAdjustedX + Margin; x++)
			{
				for (int32 PixelByte = 0; PixelByte < 3; PixelByte++)
				{
					uint32 ImageIndex = (y * WidthAdjusted + x) * 3 + PixelByte;
					uint32 RGBDataIndex = ((y - Margin) * QRWidthAdjustedX + (x - Margin)) * 3 + PixelByte;
					ImageDataPtr[ImageIndex] = RGBDataPtr[RGBDataIndex];
				}
			}
		}
		
		if (!FFileManagerGeneric::Get().DirectoryExists(*Outfile))
		{
			FFileManagerGeneric::Get().MakeDirectory(*FPaths::GetPath(Outfile), true);
		}
		FILE* BitmapFile;
		if (!(fopen_s(&BitmapFile, TCHAR_TO_UTF8(*Outfile), "wb")))
		{
			fwrite(&FileHeader, sizeof(BITMAPFILEHEADER), 1, BitmapFile);
			fwrite(&InfoHeader, sizeof(BITMAPINFOHEADER), 1, BitmapFile);
			fwrite(ImageDataPtr, sizeof(uint8), ImageBytes, BitmapFile);
			fclose(BitmapFile);
		} 
		else
		{
			printf("create file failed!!");
		}
		QRcode_free(QRCodePtr);
		free(RGBDataPtr);
		if (Margin > 0)
		{
			free(ImageDataPtr);
		}
	}
}

UTexture2D* UQRCodeBlueprintFunctionLibrary::GenerateQRCodeTexture(const int32& Width, const int32& Height, const FString& Name, int32 Margin /* = 0 */)
{
	std::string StdName(TCHAR_TO_UTF8(*Name));
	const char* QRCodeStr = StdName.c_str();
	QRcode* QRCodePtr = nullptr;
	QRCodePtr = QRcode_encodeString(QRCodeStr, 1, QR_ECLEVEL_L, QR_MODE_8, 1);
	UTexture2D* Texture = nullptr;
	if (QRCodePtr)
	{
		uint32 QRWidth, QRWidthAdjustedX, QRHeightAdjustedY, QRDataBytes;
		
		QRWidth = QRCodePtr->width;//矩阵的维数
		uint32 ScaleX = (Width - 2 * Margin) / QRWidth;
		uint32 ScaleY = (Height - 2 * Margin) / QRWidth;
		QRWidthAdjustedX = QRWidth * ScaleX;//水平维度占的像素个数（ScaleX）
		QRHeightAdjustedY = QRWidth * ScaleY;//垂直维度占的像素个数（ScaleY）
		QRDataBytes = QRWidthAdjustedX * QRHeightAdjustedY * 3;//每一个像素3个字节(BGR)

		uint8* RGBDataPtr = (uint8 *)malloc(QRDataBytes);
		if (!RGBDataPtr)
		{
			printf("out of memory!!");
			return Texture;
		}

		uint8* QRCodeSourceDatas = QRCodePtr->data;
		uint8* QRCodeDestData;
		memset(RGBDataPtr, 0xFF, QRDataBytes);	//分配内存，并且填充为白色
		for (uint32 y = 0; y < QRWidth; y++)
		{
			QRCodeDestData = RGBDataPtr + ScaleY * y * QRWidthAdjustedX * 3;
			for (uint32 x = 0; x < QRWidth; x++)
			{
				if (*QRCodeSourceDatas & 0x01)
				{
					for (uint32 rectY = 0; rectY < ScaleY; rectY++)
					{
						for (uint32 rectX = 0; rectX < ScaleX; rectX++)
						{
							*(QRCodeDestData + rectY * QRWidthAdjustedX * 3 + rectX * 3) = 0;//Blue
							*(QRCodeDestData + rectY * QRWidthAdjustedX * 3 + rectX * 3 + 1) = 0;//Green
							*(QRCodeDestData + rectY * QRWidthAdjustedX * 3 + rectX * 3 + 2) = 0;//Red
						}
					}
				}
				QRCodeDestData += ScaleX * 3;
				QRCodeSourceDatas += 1;
			}
		}

		QRcode_free(QRCodePtr);
		TArray<uint8> ImageBGRAData;
		//for (uint32 i = 0; i < QRDataBytes; i++)
		//{
		//	ImageBGRAData.Add(RGBDataPtr[i]);
		//	if ((i + 1) % 3 == 0)
		//	{
		//		ImageBGRAData.Add(0xFF);//填充Alpha通道为不透明
		//	}
		//}
		for (uint32 i = 0; i < (uint32)Width * (uint32)Height * 4; i++)
		{
			ImageBGRAData.Add(0xFF);
		}
		for (uint32 y = Margin; y < QRHeightAdjustedY + Margin; y++)
		{
			for (uint32 x = Margin; x < QRWidthAdjustedX + Margin; x++)
			{
				for (int32 PixelByte = 0; PixelByte < 3; PixelByte++)
				{
					uint32 ImageIndex = ( y * Width + x ) * 4 + PixelByte;
					uint32 RGBDataIndex = ((y - Margin) * QRWidthAdjustedX + (x - Margin)) * 3 + PixelByte;
					ImageBGRAData[ImageIndex] = RGBDataPtr[RGBDataIndex];
				}
			}
		}

		free(RGBDataPtr);
		
		IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
		TSharedPtr<IImageWrapper> TargetImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::BMP);
		if (TargetImageWrapper.IsValid())
		{
			Texture = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8);
			if (Texture != nullptr)
			{
				FTexture2DMipMap& Mip = Texture->PlatformData->Mips[0];
				void* TextureData = Mip.BulkData.Lock(LOCK_READ_WRITE);
				FMemory::Memcpy(TextureData, ImageBGRAData.GetData(), ImageBGRAData.Num());
				Mip.BulkData.Unlock();
				Texture->UpdateResource();
			}
		}
	}

	return Texture;
}

bool UQRCodeBlueprintFunctionLibrary::GenerateQRCodeImageByType(const int32& Width, const int32& Height, const FString& Name, const FString& Outfile, QR_IMAGE_FORMAT ImageFormat, int32 Margin /*= 0*/)
{
	std::string StdName(TCHAR_TO_UTF8(*Name));
	const char* QRCodeStr = StdName.c_str();
	QRcode* QRCodePtr = nullptr;
	QRCodePtr = QRcode_encodeString(QRCodeStr, 1, QR_ECLEVEL_L, QR_MODE_8, 1);
	UTexture2D* Texture = nullptr;
	if (QRCodePtr)
	{
		uint32 QRWidth, QRWidthAdjustedX, QRHeightAdjustedY, QRDataBytes;

		QRWidth = QRCodePtr->width;//矩阵的维数
		uint32 ScaleX = (Width - 2 * Margin) / QRWidth;
		uint32 ScaleY = (Height - 2 * Margin) / QRWidth;
		QRWidthAdjustedX = QRWidth * ScaleX;//水平维度占的像素个数（ScaleX）
		QRHeightAdjustedY = QRWidth * ScaleY;//垂直维度占的像素个数（ScaleY）
		QRDataBytes = QRWidthAdjustedX * QRHeightAdjustedY * 3;//每一个像素3个字节(BGR)

		uint8* RGBDataPtr = (uint8 *)malloc(QRDataBytes);
		if (!RGBDataPtr)
		{
			printf("out of memory!!");
			return false;
		}

		uint8* QRCodeSourceDatas = QRCodePtr->data;
		uint8* QRCodeDestData;
		memset(RGBDataPtr, 0xFF, QRDataBytes);	//分配内存，并且填充为白色
		for (uint32 y = 0; y < QRWidth; y++)
		{
			QRCodeDestData = RGBDataPtr + ScaleY * y * QRWidthAdjustedX * 3;
			for (uint32 x = 0; x < QRWidth; x++)
			{
				if (*QRCodeSourceDatas & 0x01)
				{
					for (uint32 rectY = 0; rectY < ScaleY; rectY++)
					{
						for (uint32 rectX = 0; rectX < ScaleX; rectX++)
						{
							*(QRCodeDestData + rectY * QRWidthAdjustedX * 3 + rectX * 3) = 0;//Blue
							*(QRCodeDestData + rectY * QRWidthAdjustedX * 3 + rectX * 3 + 1) = 0;//Green
							*(QRCodeDestData + rectY * QRWidthAdjustedX * 3 + rectX * 3 + 2) = 0;//Red
						}
					}
				}
				QRCodeDestData += ScaleX * 3;
				QRCodeSourceDatas += 1;
			}
		}

		QRcode_free(QRCodePtr);
		TArray<uint8> ImageBGRAData;

		for (uint32 i = 0; i < (uint32)Width * (uint32)Height * 4; i++)
		{
			ImageBGRAData.Add(0xFF);
		}
		for (uint32 y = Margin; y < QRHeightAdjustedY + Margin; y++)
		{
			for (uint32 x = Margin; x < QRWidthAdjustedX + Margin; x++)
			{
				for (int32 PixelByte = 0; PixelByte < 3; PixelByte++)
				{
					uint32 ImageIndex = (y * Width + x) * 4 + PixelByte;
					uint32 RGBDataIndex = ((y - Margin) * QRWidthAdjustedX + (x - Margin)) * 3 + PixelByte;
					ImageBGRAData[ImageIndex] = RGBDataPtr[RGBDataIndex];
				}
			}
		}

		free(RGBDataPtr);

		IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
		TSharedPtr<IImageWrapper> TargetImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat(ImageFormat));
		if (TargetImageWrapper.IsValid())
		{
			if (TargetImageWrapper->SetRaw(ImageBGRAData.GetData(), ImageBGRAData.Num(), Width, Height, ERGBFormat::BGRA, 8))
			{
				TArray<uint8> TagetImageData = TargetImageWrapper->GetCompressed();
				if (!FFileManagerGeneric::Get().DirectoryExists(*Outfile))
				{
					FFileManagerGeneric::Get().MakeDirectory(*FPaths::GetPath(Outfile), true);
				}
				return FFileHelper::SaveArrayToFile(TagetImageData, *Outfile);
			}
		}
	}
	return false;
}
