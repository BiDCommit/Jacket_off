#ifndef SmartReliefPro_H
#define SmartReliefPro_H
#include <Arduino.h>

// Частота ШИМ
const int frequency = 5000;
// Количество каналов для ШИМ
const int numChannels = 20;
// Разрешение ШИМ (в битах)
const int resolution = 8;
u_int8_t stren=0;
#define maxIndex 26

// Результаты функции split
String splitResult[6];
// Значения силы вибрации

// Коэффициенты для изменения периода импульсов
u_int8_t maxvalue = 40;

// Структура для хранения состояния импульсов
struct PulseState {
    int currentIndex; // Текущий индекс в массиве коэффициентов
    int currentStrength; // Текущая сила вибрации
};

// Массив структур PulseState для каждого канала
PulseState pulseStates[numChannels];

// Массивы значений силы вибрации для каждого канала
int pulseStrengths[numChannels][maxIndex] = {
    {255, 200, 255, 200, 255, 200, 255, 200, 255, 200, 255, 200, 255, 200, 255, 200, 255, 200, 255, 200, 255, 200, 255, 200, 255, 200},
    {20, 255, 230, 210, 190, 170, 150, 130, 110, 90, 70, 50, 30, 10, 40, 60, 80, 100, 120, 140, 160, 180, 200, 220, 240, 250},
    {5, 245, 235, 225, 215, 205, 195, 185, 175, 165, 155, 145, 135, 125, 115, 105, 95, 85, 75, 65, 55, 45, 35, 25, 15, 250},
    {15, 130, 200, 100, 180, 90, 150, 220, 50, 120, 255, 70, 140, 190, 110, 170, 150, 210, 80, 220, 250, 160, 130, 140, 200, 170},
    {25, 110, 150, 90, 200, 60, 170, 210, 50, 130, 230, 30, 100, 255, 40, 80, 140, 70, 190, 120, 210, 160, 140, 250, 130, 120},
    {10, 255, 200, 180, 150, 140, 120, 100, 80, 60, 40, 30, 20, 200, 180, 150, 140, 120, 100, 80, 60, 40, 30, 20, 150, 100},
    {30, 255, 225, 195, 165, 135, 105, 75, 45, 15, 60, 90, 120, 150, 180, 210, 240, 250, 130, 170, 190, 110, 160, 150, 200, 220},
    {50, 255, 205, 155, 105, 200, 180, 150, 100, 220, 120, 140, 250, 200, 220, 150, 250, 130, 170, 190, 110, 160, 150, 200, 220, 240},
    {70, 255, 235, 215, 195, 175, 155, 135, 115, 95, 75, 55, 35, 15, 45, 65, 85, 105, 125, 145, 165, 185, 205, 225, 245, 250},
    {90, 185, 170, 150, 130, 255, 200, 185, 150, 220, 180, 210, 250, 120, 140, 200, 170, 190, 110, 160, 150, 200, 220, 240, 130, 170},
    {60, 220, 200, 185, 255, 150, 130, 100, 170, 210, 185, 230, 200, 160, 130, 180, 250, 190, 220, 210, 130, 170, 190, 110, 160, 150},
    {80, 255, 150, 185, 130, 170, 250, 200, 220, 210, 240, 180, 200, 160, 130, 180, 250, 190, 220, 210, 130, 170, 190, 110, 160, 150},
    {100, 255, 200, 185, 130, 170, 250, 210, 240, 180, 200, 160, 130, 180, 250, 190, 220, 210, 130, 170, 190, 110, 160, 150, 200, 240},
    {120, 150, 185, 220, 255, 200, 170, 210, 150, 140, 240, 180, 200, 160, 130, 180, 250, 190, 220, 210, 130, 170, 190, 110, 160, 150}
};

void updatePulseState(PulseState &state, int *strengths, int numStrengths, int channel) {
    float coefficient = 255.0 / 40.0;
    for (int i = 0; i < numStrengths; ++i) {
        strengths[i] = (int)(coefficient * (float)strengths[i] * maxvalue / 255.0);
    }
    state.currentStrength = strengths[state.currentIndex];
    stren=state.currentStrength;
    ledcWrite(channel, state.currentStrength);
    state.currentIndex = (state.currentIndex + 1) % numStrengths;
}

void callPulseFunction(int pulseNumber, int channel) {
    if (pulseNumber < 1 || pulseNumber > 14 || channel < 0 || channel >= numChannels) {
        return; // Неверный номер пульсации или канала
    }
    int *strengths = pulseStrengths[pulseNumber - 1];
    int numStrengths = sizeof(pulseStrengths[pulseNumber - 1]) / sizeof(pulseStrengths[pulseNumber - 1][0]);
    updatePulseState(pulseStates[channel], strengths, numStrengths, channel);
}

#endif


/*
#ifndef SmartReliefPro_H
#define SmartReliefPro_H
#include <Arduino.h>

// Частота ШИМ
const int frequency = 5000;
// Канал для ШИМ
const int ledChannel = 0;
// Разрешение ШИМ (в битах)
const int resolution = 8;

#define maxIndex 26

// Результаты функции split
String splitResult[6];
// Значения силы вибрации

// Коэффициенты для изменения периода импульсов
u_int8_t maxvalue=40;

// Структура для хранения состояния импульсов
struct PulseState {
    int currentIndex; // Текущий индекс в массиве коэффициентов
    int currentStrength; // Текущая сила вибрации
};

PulseState pulseStates[20];



void updatePulseState(PulseState &state, int *strengths, int numStrengths, int channel) {

        float coefficient = 255.0 / 40.0;
     for (int i = 0; i < numStrengths; ++i) {
 strengths[i] = (int)(coefficient * (float)strengths[i] * maxvalue / 255.0);
// Serial.print(strengths[i]);Serial.print(",");
    }
 //Serial.println("");
     state.currentStrength = strengths[state.currentIndex];
    ledcWrite(channel, state.currentStrength);
    state.currentIndex = (state.currentIndex + 1) % numStrengths;
}

void pulse1_FastInSlowOut(int channel) {
    int strengths[] = {0, 150, 185, 220, 255, 220, 185, 150, 0, 150, 185, 220, 255, 220, 185, 150, 0};
    updatePulseState(pulseStates[0], strengths, sizeof(strengths) / sizeof(strengths[0]), channel);
}

void pulse3_SOSBlink(int channel) {
    int strengths[] = {0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255};
    updatePulseState(pulseStates[1], strengths, sizeof(strengths) / sizeof(strengths[0]), channel);
}

void pulse4_BlinkThrice(int channel) {
    int strengths[] = {0, 255, 0, 255, 0, 255, 0};
    updatePulseState(pulseStates[2], strengths, sizeof(strengths) / sizeof(strengths[0]), channel);
}

void pulse5_FadeIn(int channel) {
    int strengths[] = {0, 150, 185, 220, 255, 0};
    updatePulseState(pulseStates[3], strengths, sizeof(strengths) / sizeof(strengths[0]), channel);
}

void pulse8_GradualBuild(int channel) {
    int strengths[] = {0, 150, 185, 220, 255, 0, 150, 185, 220, 255, 0};
    updatePulseState(pulseStates[7], strengths, sizeof(strengths) / sizeof(strengths[0]), channel);
}

void pulse10_DarkFlash(int channel) {
    int strengths[] = {0, 255, 0, 255, 0};
    updatePulseState(pulseStates[9], strengths, sizeof(strengths) / sizeof(strengths[0]), channel);
}

void pulse11_BlinkDecreasing(int channel) {
    int strengths[] = {0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0};
    updatePulseState(pulseStates[10], strengths, sizeof(strengths) / sizeof(strengths[0]), channel);
}

void pulse12_Heartbeat(int channel) {
    int strengths[] = {0, 255, 0, 255, 0, 255, 0, 255, 0};
    updatePulseState(pulseStates[11], strengths, sizeof(strengths) / sizeof(strengths[0]), channel);
}

void pulse13_BlinkIncreasing(int channel) {
    int strengths[] = {0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0};
    updatePulseState(pulseStates[12], strengths, sizeof(strengths) / sizeof(strengths[0]), channel);
}

void pulse14_Raindrops(int channel) {
    int strengths[] = {0, 185, 0, 150, 0, 255, 0, 185, 0, 255, 0, 150, 0};
    updatePulseState(pulseStates[13], strengths, sizeof(strengths) / sizeof(strengths[0]), channel);
}

void pulse16_TransmissionRandomBrightness(int channel) {
    int strengths[] = {220, 0, 185, 0, 255, 0, 150, 0, 220, 0, 185, 0, 255, 0};
    updatePulseState(pulseStates[15], strengths, sizeof(strengths) / sizeof(strengths[0]), channel);
}

void pulse17_Lightning(int channel) {
    int strengths[] = {0, 255, 150, 185, 0, 150, 0, 255, 150, 185, 0, 150, 0};
    updatePulseState(pulseStates[16], strengths, sizeof(strengths) / sizeof(strengths[0]), channel);
}

void pulse18_TransmissionFixedBrightness(int channel) {
    int strengths[] = {255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0};
    updatePulseState(pulseStates[17], strengths, sizeof(strengths) / sizeof(strengths[0]), channel);
}

void pulse19_StaircaseContinuous(int channel) {
    int strengths[] = {0, 150, 185, 220, 255, 0};
    updatePulseState(pulseStates[18], strengths, sizeof(strengths) / sizeof(strengths[0]), channel);
}
void callPulseFunction(int pulseNumber, int channel) {
    switch (pulseNumber) {
        case 1:
            pulse1_FastInSlowOut(channel);
            break;
        case 2:
            pulse3_SOSBlink(channel);
            break;
        case 3:
            pulse4_BlinkThrice(channel);
            break;
        case 4:
            pulse5_FadeIn(channel);
            break;
        case 5:
            pulse8_GradualBuild(channel);
            break;
        case 6:
            pulse10_DarkFlash(channel);
            break;
        case 7:
            pulse11_BlinkDecreasing(channel);
            break;
        case 8:
            pulse12_Heartbeat(channel);
            break;
        case 9:
            pulse13_BlinkIncreasing(channel);
            break;
        case 10:
            pulse14_Raindrops(channel);
            break;
        case 11:
            pulse16_TransmissionRandomBrightness(channel);
            break;
        case 12:
            pulse17_Lightning(channel);
            break;
        case 13:
            pulse18_TransmissionFixedBrightness(channel);
            break;
        case 14:
            pulse19_StaircaseContinuous(channel);
            break;
        default:
            // Если передан неверный номер, можно сделать что-то по умолчанию, например, ничего не делать
            break;
    }
}
#endif*/