# FlashLoader demo projekt
Det kan godt være lidt langhåret at få STLink til at accepterer en hjemmelavet Flash Loader .stldr fil

Dette projekt fungerer med en STM32U535RET LQFP64 processor og IS25LP512M 64 MByte QSPI Flash fra ISSI

Projektet er udviklet til intern brug, og er ikke nødvendigvis dokumenteret på samme niveau som et projekt til distribution.

Projektet hedder CMFlashLoader fordi det er en FlashLoader til et board der kaldes Control Module (derfor CM:-)

## Hardware setup
QSPI flashen er forbundet til følgende processor ben:

QSPI_NCE PA2
QSPI_SCK PA3
QSPI_IO3 PA6
QSPI_IO2 PA7
QSPI_IO1 PB0
QSPI_IO0 PB1

USART3 benyttes som Debug port med TX på PC4 og RX på PC5. Og er sat op til 921.600 Baud

## Develop environment
STM32CubeMX filen CMFlashLoader.ioc definer hardware interfacet. Er senest kompileret med version 6.13.0 af STM32CubeMX.

Efterfølgende er programmet udviklet i STM32CubeIDE Version 1.18.1.

## Configurations
Projektet kan køres i 2 forskellige Configurations: Debug eller FlashLoader

I Debug konfigurationen loades programmet i processorens interne flash, og starte via main(void)
styret af linker filen (STM32U535RETX_FLASH.ld).
Så her kan man debug'ge programmet og få testet flash interfacet.

Hvis man så oversætter med FlashLoader konfigurationen, genereres en .stldr fil der kan bruges af STLink.
Denne loades af STLink i processor intern RAM, og STLink kalder diverse funktioner, der ligger på specifikke adresser. Dette 
er styret af linker filen (STM32U535RETX_FLASH_LOADER.ld)

Den resulterende .stldr fil kopieres automatisk til stldr_files folderen i projekt roden, efter hver build.

Efter build kommer følgende warning:
"warning: CMFlashLoader.elf has a LOAD segment with RWX permissions"
Men det har ingen betydning, og er en kendt ny ting der kom ind i en af de seneste updates af STM32CubeIDE.

Normalt er den største udfordring at få QSPI interfacet til at fungerer, da de forskellige typer Flash kredse næsten 
altid er lidt forskellige i programmerings interfacet. Så det er en god ide at læse og helst forstå databladet for 
den specifikke Flash kreds.

Jeg har efterhånder udviklet ganske mange FlashLoader'er til forskellige kunde projekter, baseret på dette framework.

Jeg har givetvis glemt at beskrive alle finurlighederne i dette projekt, så hvis I har spørgsmål, er i meget 
velkomne til at kontakte mig:

Klaus Karkov
KK Data Danmark ApS
e-mail: klaus.karkov@kdata.dk
Mobil: 40 84 08 33
