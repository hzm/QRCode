# QRCode
create QRCode in UE4(Tested in 4.21 Editor and Game)

use UQRCodeBlueprintFunctionLibrary::GenerateQRCodeTexture to create dynamic QRCode Texture.

use UQRCodeBlueprintFunctionLibrary::GenerateQRCodeBitmap to create dynamic QRcode and save as bmp.

use UQRCodeBlueprintFunctionLibrary::GenerateQRCodeImageByType to create dynamic QRcode and save as png or jpg.

# Instructions for use
1. create Plugins directory
2. git clone https://github.com/hzm/QRCode.git
3. Put the folder in the plugins directory
4. Enable this plugins in the editor
5. Generate visual studio project files
6. Add the QRcode module to the project's buid.cs file. such as：
PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "QRcode" });



# Special Thanks
ThirdParty：
libqrencode 4.0.2（https://github.com/fukuchi/libqrencode）

reference：
https://wiki.unrealengine.com/Dynamic_Textures
https://blog.csdn.net/kupepoem/article/details/43307387
https://blog.csdn.net/u014532636/article/details/77848185




