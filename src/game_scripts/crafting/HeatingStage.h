#pragma once

#include <spdlog/spdlog.h>

namespace Crafting{
    class HeatingStage{
    public:
        float requiredGoodTemperatureTime = 6.0f;

        float tempPerClick = 6.0f;

        float clickCooldown = 1.1f;

        float coolingDelay = 0.35f;

        float coolingSpeed = 2.0f;

        float tempMin = 45.0f;
        float tempMax = 57.0f;

        float qualityLoss = 4.0f;

        float progressLoss = 0.5f;

        void Reset(){
            temperature = 0.0f;
            qualityPercent = 0.0f;

            totalTimer = 0.0f;
            goodTemperatureTimer = 0.0f;
            coolingDelayTimer = 0.0f;
            blowerCooldownTimer = 0.0f;

            finished = false;
            wasInPerfectRange = false;
            wasTooHot = false;
            lastLoggedGoodSecond = -1;
        }

        void Start(){
            temperature = 0.0f;
            qualityPercent = 100.0f;

            totalTimer = 0.0f;
            goodTemperatureTimer = 0.0f;
            coolingDelayTimer = 0.0f;
            blowerCooldownTimer = 0.0f;

            finished = false;

            wasInPerfectRange = false;
            wasTooHot = false;
            lastLoggedGoodSecond = -1;

            spdlog::info(
                "Heating: start | range {}-{} | required {}s | quality {}%",
                tempMin,
                tempMax,
                requiredGoodTemperatureTime,
                qualityPercent
            );
        }

        bool Update(float deltaTime, bool blowerClicked){
            if (finished){
                return true;
            }

            totalTimer += deltaTime;

            UpdateBlowerCooldown(deltaTime);

            if (blowerClicked){
                TryUseBlower();
            }

            UpdateTemperature(deltaTime);
            UpdateQualityAndProgress(deltaTime);
            LogImportantStateChanges();

            if (goodTemperatureTimer >= requiredGoodTemperatureTime){
                finished = true;

                spdlog::info(
                    "Heating: finished | quality {}% | temp {} | time {}s",
                    qualityPercent,
                    temperature,
                    totalTimer
                );
            }

            return finished;
        }

        bool CanClickBlower() const{
            return !finished && blowerCooldownTimer <= 0.0f;
        }

        float GetBlowerCooldownRemaining() const{
            return blowerCooldownTimer;
        }

        float GetBlowerCooldown01() const{
            if (clickCooldown <= 0.0f){
                return 0.0f;
            }

            return Clamp(blowerCooldownTimer / clickCooldown, 0.0f, 1.0f);
        }

        float GetTemperature() const{
            return temperature;
        }

        float GetQuality() const{
            return qualityPercent;
        }

        float GetQualityPercent() const{
            return qualityPercent;
        }

        float GetQuality01() const{
            return Clamp(qualityPercent / 100.0f, 0.0f, 1.0f);
        }

        float GetTotalTimer() const{
            return totalTimer;
        }

        float GetGoodTemperatureTimer() const{
            return goodTemperatureTimer;
        }

        float GetProgress01() const{
            if (requiredGoodTemperatureTime <= 0.0f){
                return 1.0f;
            }

            return Clamp(
                goodTemperatureTimer / requiredGoodTemperatureTime,
                0.0f,
                1.0f
            );
        }

        float GetProgressPercent() const{
            return GetProgress01() * 100.0f;
        }

        bool IsFinished() const{
            return finished;
        }

        bool IsInPerfectRange() const{
            return
                temperature >= tempMin &&
                temperature <= tempMax;
        }

        bool IsTooHot() const{
            return temperature > tempMax;
        }

        bool IsTooCold() const{
            return temperature < tempMin;
        }

    private:
        float temperature = 0.0f;
        float qualityPercent = 100.0f;

        float totalTimer = 0.0f;
        float goodTemperatureTimer = 0.0f;
        float coolingDelayTimer = 0.0f;
        float blowerCooldownTimer = 0.0f;

        bool finished = false;

        bool wasInPerfectRange = false;
        bool wasTooHot = false;
        int lastLoggedGoodSecond = -1;

        void UpdateBlowerCooldown(float deltaTime){
            if (blowerCooldownTimer <= 0.0f){
                return;
            }

            blowerCooldownTimer -= deltaTime;

            if (blowerCooldownTimer < 0.0f){
                blowerCooldownTimer = 0.0f;
            }
        }

        void TryUseBlower(){
            if (!CanClickBlower()){
                spdlog::info(
                    "Heating: blower blocked | cooldown {:.2f}s",
                    blowerCooldownTimer
                );

                return;
            }

            temperature += tempPerClick;
            temperature = Clamp(temperature, 0.0f, 100.0f);

            coolingDelayTimer = coolingDelay;
            blowerCooldownTimer = clickCooldown;

            spdlog::info(
                "Heating: blower used | temp {} | next {:.2f}s",
                temperature,
                clickCooldown
            );
        }

        void UpdateTemperature(float deltaTime){
            if (coolingDelayTimer > 0.0f){
                coolingDelayTimer -= deltaTime;

                if (coolingDelayTimer < 0.0f){
                    coolingDelayTimer = 0.0f;
                }

                return;
            }

            if (temperature > 0.0f){
                temperature -= coolingSpeed * deltaTime;
            }

            temperature = Clamp(temperature, 0.0f, 100.0f);
        }

        void UpdateQualityAndProgress(float deltaTime){
            if (IsInPerfectRange()){
                goodTemperatureTimer += deltaTime;
            }else if (IsTooCold()){
                goodTemperatureTimer -= progressLoss * deltaTime;
            }

            if (IsTooHot()){
                qualityPercent -= qualityLoss * deltaTime;
            }

            qualityPercent = Clamp(qualityPercent, 0.0f, 100.0f);

            goodTemperatureTimer =
                Clamp(goodTemperatureTimer, 0.0f, requiredGoodTemperatureTime);
        }

        void LogImportantStateChanges(){
            bool isInPerfectRange = IsInPerfectRange();
            bool isTooHot = IsTooHot();
            bool isTooCold = IsTooCold();

            if (isInPerfectRange && !wasInPerfectRange){
                spdlog::info(
                    "Heating: good range entered | temp {}",
                    temperature
                );
            }

            if (isTooHot && !wasTooHot){
                spdlog::warn(
                    "Heating: too hot | temp {} | quality decreasing",
                    temperature
                );
            }

            if (!isTooHot && wasTooHot){
                spdlog::info(
                    "Heating: back below max | temp {} | quality {}%",
                    temperature,
                    qualityPercent
                );
            }

            int currentGoodSecond =
                static_cast<int>(goodTemperatureTimer);

            if (
                currentGoodSecond != lastLoggedGoodSecond &&
                currentGoodSecond > 0
            ){
                lastLoggedGoodSecond = currentGoodSecond;

                spdlog::info(
                    "Heating: progress {}/{}s | quality {}% | temp {}",
                    currentGoodSecond,
                    static_cast<int>(requiredGoodTemperatureTime),
                    qualityPercent,
                    temperature
                );
            }

            wasInPerfectRange = isInPerfectRange;
            wasTooHot = isTooHot;
        }

        float Clamp(float value, float minValue, float maxValue) const{
            if (value < minValue){
                return minValue;
            }

            if (value > maxValue){
                return maxValue;
            }

            return value;
        }
    };
}