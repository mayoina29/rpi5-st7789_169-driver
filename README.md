> [!NOTE]
> 이 프로젝트는 개인 실습용이므로 소스 파일명이 `myspi.c`로 되어 있습니다. 
> 빌드 시 `myspi.ko`가 생성되니 참고해 주세요.
> 권한 문제가 발생할수 있습니다. 문제가 생긴다면 로그인 유저에게 허용해주세요
> 라즈베리파이에 ssh로 접속해서 프로젝트 진행하였습니다. 


# ST7789V2 Linux Framebuffer Driver for Raspberry Pi 5

라즈베리파이 5에서 ST7789V2 SPI LCD (1.69인치, 240x280)를 구동하기 위한 **리눅스 프레임버퍼 드라이버(Linux Framebuffer Driver)**입니다.

단순한 SPI 전송을 넘어, 리눅스 커널의 **Deferred IO** 기능을 사용하여 효율적인 화면 갱신을 지원하며, 표준 `/dev/fbX` 인터페이스를 제공합니다.

## 주요 기능 (Features)
* **표준 프레임버퍼 지원:** `/dev/fb0` (또는 fb1) 장치 생성
* **Deferred IO 구현:** VRAM 변경 사항을 감지하여 자동으로 SPI 전송 (CPU 부하 감소)
* **가상 메모리(VRAM) 매핑:** `vmalloc`을 사용한 커널 메모리 할당 및 `mmap` 지원
* **고속 SPI 전송:** 화면 갱신 최적화 및 Endianness (RGB565) 자동 변환
* **호환성:** `fbi` (이미지 뷰어)

## 하드웨어 연결 (Wiring)

| ST7789 Pin | Raspberry Pi 5 Pin | BCM (GPIO) | Function |
| :--- | :--- | :--- | :--- |
| **VCC** | Pin 1 or 17 | 3.3V | Power |
| **GND** | Pin 6 or 9 | GND | Ground |
| **SCL** (SCLK) | Pin 23 | GPIO 11 | SPI0 SCLK |
| **SDA** (MOSI) | Pin 19 | GPIO 10 | SPI0 MOSI |
| **RES** (RST) | Pin 22 | GPIO 25 | Reset |
| **DC** | Pin 18 | GPIO 24 | Data/Command |
| **CS** | Pin 24 | GPIO 8 | SPI0 CE0 |
| **BLK** | - | - | Backlight (VCC) |




## 🛠️ 빌드 및 설치 (Build & Install)

## Device Tree 설정 
이 드라이버가 작동하려면 **Device Tree Overlay**를 통해 커널에 ST7789 LCD 연결되었다는것을 알려줘야합니다. 

### 1.컴파일 및 설치
Device Tree 컴파일러(`dtc`)를 사용하여 `.dtbo` 파일을 만들고 시스템 폴더로 복사합니다.

```bash
# 1. dts를 dtbo로 컴파일
dtc -I dts -O dtb -o myspi.dtbo myspi.dts

# 2. 시스템 오버레이 폴더로 복사
sudo cp myspi.dtbo /boot/firmware/overlays/
```
### 2.부트로더 설정 (`congif.txt`)
`/boot/firmware/config.txt` 파일을 열어 오버레이를 활성화합니다.

```bash
sudo vim /boot/firmware/config.txt
```

파일 맨 아래에 다음 두 줄을 추가하고 재부팅합니다. 

```ini
dtparam=spi=on
dtoverlay=myspi
```

### 3. 재부팅

```bash
sudo reboot 
```

## 드라이버 올리기 insmod

### 1. 필수 패키지 설치
커널 헤더 파일이 필요합니다.
```bash
sudo apt install raspberrypi-kernel-headers
```

### 2. 컴파일 (Make)
```bash
make
```

### 3. 드라이버 로드 (Load Module)
```bash
sudo insmod myspi.ko
```

### 4. 설치 확인
`/dev/fbX` 장치가 생성되었는지 확인합니다.
```bash
ls -l /dev/fb*
dmesg | grep ST7789
```

## 사용 예제 (Usage)
### 이미지 띄우기 (fbi)
fbi 툴을 사용하여 이미지를 출력합니다.

```bash
sudo apt install fbi
sudo fbi -d /dev/fb0 -T 1 -noverbose -a image.jpg
```

## 개발 환경 (Environment)
Hardware: Raspberry Pi 5 (RP1 Architecture)

OS: Raspberry Pi OS (64-bit)

Kernel: Linux 6.12.x

Display: ST7789V2 1.69" IPS LCD (240x280)

## 라이선스 (License)
GPL v2
