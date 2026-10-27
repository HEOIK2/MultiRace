#pragma once

#include "CoreMinimal.h"
#include "WheeledVehiclePawn.h"
#include "CXPawn.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UInputAction;
class UChaosWheeledVehicleMovementComponent;
class UParticleSystem;
class USoundBase;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateVehicle, Log, All);

UCLASS(abstract)
class ACXPawn : public AWheeledVehiclePawn
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* FrontSpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FrontCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* BackSpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* BackCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CX|Fire", meta = (AllowPrivateAccess = "true"))
	USceneComponent* FireStartPoint;

	TObjectPtr<UChaosWheeledVehicleMovementComponent> ChaosVehicleMovement;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* SteeringAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* ThrottleAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* BrakeAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* HandbrakeAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* LookAroundAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* ToggleCameraAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* ResetVehicleAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* FireAction;

	bool bFrontCameraActive = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CX|Speed")
	float BaseMaxSpeed = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CX|Fire")
	float FireRange = 3000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CX|Fire")
	float FireCooldown = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CX|Fire")
	float FireDamage = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CX|Fire")
	int32 StealCoinAmount = 5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CX|Fire")
	TObjectPtr<UParticleSystem> FireHitEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CX|Fire")
	TObjectPtr<USoundBase> FireHitSound;

	float ThrottleInput = 0.0f;

	float LastFireTime = -1000.0f;

public:
	ACXPawn();

	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;

	virtual void Tick(float Delta) override;

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

protected:
	void Steering(const FInputActionValue& Value);

	void Throttle(const FInputActionValue& Value);

	void Brake(const FInputActionValue& Value);

	void StartBrake(const FInputActionValue& Value);
	void StopBrake(const FInputActionValue& Value);

	void StartHandbrake(const FInputActionValue& Value);
	void StopHandbrake(const FInputActionValue& Value);

	void LookAround(const FInputActionValue& Value);

	void ToggleCamera(const FInputActionValue& Value);

	void ResetVehicle(const FInputActionValue& Value);

	void Fire(const FInputActionValue& Value);

	bool CanFire() const;

	float GetMaxSpeed() const;

	void ApplyThrottleInput();

	UFUNCTION(Server, Reliable)
	void Server_Fire();

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayFireEffect(FVector_NetQuantize Start, FVector_NetQuantize End, bool bHitVehicle);

	UFUNCTION(BlueprintImplementableEvent, Category="Vehicle")
	void BrakeLights(bool bBraking);

public:
	FORCEINLINE USpringArmComponent* GetFrontSpringArm() const { return FrontSpringArm; }

	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FrontCamera; }

	FORCEINLINE USpringArmComponent* GetBackSpringArm() const { return BackSpringArm; }

	FORCEINLINE UCameraComponent* GetBackCamera() const { return BackCamera; }

	FORCEINLINE const TObjectPtr<UChaosWheeledVehicleMovementComponent>& GetChaosVehicleMovement() const { return ChaosVehicleMovement; }
};
