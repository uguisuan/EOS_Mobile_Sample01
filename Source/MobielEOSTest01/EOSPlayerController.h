// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "EOSPlayerController.generated.h"

/**
 *
 */
UCLASS()
class MOBIELEOSTEST01_API AEOSPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Qiita Sample")
		void Login();

	UFUNCTION(BlueprintCallable, Category = "Qiita Sample")
		bool HostSession();

	UFUNCTION(BlueprintCallable, Category = "Qiita Sample")
		void FindSession();

	UFUNCTION(BlueprintCallable, Category = "Qiita Sample")
		void KillSession();

private:
	void OnLoginCompleteDelegate(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error);
	void JoinSession(FOnlineSessionSearchResult SearchResult);
	void OnJoinSessionCompleteDelegate(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnCreateSessionCompleteDelegate(FName InSessionName, bool bWasSuccessful);
	void OnFindSessionsCompleteDelegate(bool bWasSuccessful);
};
