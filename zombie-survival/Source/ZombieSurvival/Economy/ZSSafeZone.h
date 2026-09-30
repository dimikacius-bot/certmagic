#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/TimerHandle.h"
#include "ZSSafeZone.generated.h"

class UBoxComponent;
class USceneComponent;

/**
 * A fortified camp: place merchants inside. Zombies that stray in are shot by the guards, nothing
 * spawns nearby, and zombies lose interest in players standing inside. Dying respawns you at the
 * last safe zone you entered. Works with World Partition (it registers itself while loaded).
 */
UCLASS()
class ZOMBIESURVIVAL_API AZSSafeZone : public AActor
{
	GENERATED_BODY()

public:
	AZSSafeZone();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Safe Zone")
	FText ZoneName = FText::FromString(TEXT("Safe Zone"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Safe Zone")
	bool bGuardsKillZombies = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Safe Zone", meta = (EditCondition = "bGuardsKillZombies"))
	float GuardDamage = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Safe Zone", meta = (EditCondition = "bGuardsKillZombies", Units = "s"))
	float GuardInterval = 0.75f;

	/** Oriented-box containment test (ignores the box's scale, uses its extent). */
	bool ContainsPoint(const FVector& WorldPoint, float Margin = 0.f) const;

	FTransform GetRespawnTransform() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void OnZoneBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnZoneEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void GuardTick();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBoxComponent> Zone;

	/** Where the player wakes up after dying. Move it in the editor. */
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> RespawnPoint;

	FTimerHandle GuardTimer;
};
