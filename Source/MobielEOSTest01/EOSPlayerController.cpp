// Copyright Epic Games, Inc. All Rights Reserved.


#include "EOSPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"

const FName SESSION_NAME = "SessionName";

TSharedPtr<class FOnlineSessionSearch> SearchSettings;

void AEOSPlayerController::Login()
{
	IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
	if (OnlineSub)
	{
		IOnlineIdentityPtr Identity = OnlineSub->GetIdentityInterface();
		if (Identity.IsValid())
		{
			ULocalPlayer* LP = Cast<ULocalPlayer>(Player);
			if (LP != NULL)
			{
				int ControllerId = LP->GetControllerId();
				if (Identity->GetLoginStatus(ControllerId) != ELoginStatus::LoggedIn)
				{
					UE_LOG_ONLINE(Warning, TEXT("CommandLine: %s"), FCommandLine::Get());

					Identity->AddOnLoginCompleteDelegate_Handle(ControllerId, FOnLoginCompleteDelegate::CreateUObject(this, &AEOSPlayerController::OnLoginCompleteDelegate));
					FOnlineAccountCredentials credentials = {};
					credentials.Type = "accountportal";
					Identity->Login(ControllerId, credentials);
				}
				ELoginStatus::Type status = Identity->GetLoginStatus(ControllerId);
				UE_LOG_ONLINE(Log, TEXT("Login Status: %s"), ELoginStatus::ToString(status));
			}
		}
	}
}

void AEOSPlayerController::OnLoginCompleteDelegate(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
{
	IOnlineIdentityPtr Identity = Online::GetIdentityInterface();
	if (Identity.IsValid())
	{
		ULocalPlayer* LP = Cast<ULocalPlayer>(Player);
		if (LP != NULL)
		{
			int ControllerId = LP->GetControllerId();
			FUniqueNetIdRepl uniqueId = PlayerState->GetUniqueId();
			uniqueId.SetUniqueNetId(FUniqueNetIdWrapper(UserId).GetUniqueNetId());
			PlayerState->SetUniqueId(uniqueId);
		}
	}
}
bool AEOSPlayerController::HostSession()
{
	IOnlineSubsystem* const OnlineSub = Online::GetSubsystem(GetWorld());
	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			TSharedPtr<class FOnlineSessionSettings> SessionSettings = MakeShareable(new FOnlineSessionSettings());
			SessionSettings->NumPublicConnections = 4;
			SessionSettings->NumPrivateConnections = 0;
			SessionSettings->bShouldAdvertise = true;
			SessionSettings->bAllowJoinInProgress = true;
			SessionSettings->bAllowInvites = true;
			SessionSettings->bUsesPresence = true;
			SessionSettings->bAllowJoinViaPresence = true;
			SessionSettings->bUseLobbiesIfAvailable = true;
			SessionSettings->bUseLobbiesVoiceChatIfAvailable = true;
			SessionSettings->Set(SEARCH_KEYWORDS, FString("Custom"), EOnlineDataAdvertisementType::ViaOnlineService);

			Sessions->AddOnCreateSessionCompleteDelegate_Handle(FOnCreateSessionCompleteDelegate::CreateUObject(this, &AEOSPlayerController::OnCreateSessionCompleteDelegate));

			TSharedPtr<const FUniqueNetId> UniqueNetIdptr = GetLocalPlayer()->GetPreferredUniqueNetId().GetUniqueNetId();
			bool bResult = Sessions->CreateSession(*UniqueNetIdptr, SESSION_NAME, *SessionSettings);

			if (bResult) {
				UE_LOG_ONLINE(Log, TEXT("CreateSession: Success"), NULL);
				return true;
			}
			else {
				UE_LOG_ONLINE(Log, TEXT("CreateSession: Fail"), NULL);
			}

		}
	}

	return false;
}

void AEOSPlayerController::OnCreateSessionCompleteDelegate(FName InSessionName, bool bWasSuccessful)
{
	if (bWasSuccessful) {
		UGameplayStatics::OpenLevel(this, FName(TEXT("/Game/FirstPerson/Maps/FirstPersonMap")), true, "listen");
	}
}

void AEOSPlayerController::FindSession()
{
	IOnlineSubsystem* const OnlineSub = Online::GetSubsystem(GetWorld());
	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			SearchSettings = MakeShareable(new FOnlineSessionSearch());
			SearchSettings->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);
			SearchSettings->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
			SearchSettings->QuerySettings.Set(SEARCH_KEYWORDS, FString("Custom"), EOnlineComparisonOp::Equals);

			Sessions->AddOnFindSessionsCompleteDelegate_Handle(FOnFindSessionsCompleteDelegate::CreateUObject(this, &AEOSPlayerController::OnFindSessionsCompleteDelegate));

			TSharedRef<FOnlineSessionSearch> SearchSettingsRef = SearchSettings.ToSharedRef();
			TSharedPtr<const FUniqueNetId> UniqueNetIdptr = GetLocalPlayer()->GetPreferredUniqueNetId().GetUniqueNetId();
			bool bIsSuccess = Sessions->FindSessions(*UniqueNetIdptr, SearchSettingsRef);
		}
	}
}

void AEOSPlayerController::OnFindSessionsCompleteDelegate(bool bWasSuccessful) {
	if (bWasSuccessful) {
		UE_LOG_ONLINE(Log, TEXT("Find Session: Success"), NULL);
		if (SearchSettings->SearchResults.Num() == 0) {
			UE_LOG_ONLINE(Log, TEXT("No session found."), NULL);
		}
		else {
			const TCHAR* SessionId = *SearchSettings->SearchResults[0].GetSessionIdStr();
			UE_LOG_ONLINE(Log, TEXT("Session ID: %s"), SessionId);
			JoinSession(SearchSettings->SearchResults[0]);
		}
	}
	else {
		UE_LOG_ONLINE(Log, TEXT("Find Session: Fail"), NULL);
	}
}

void AEOSPlayerController::JoinSession(FOnlineSessionSearchResult SearchResult) {
	IOnlineSubsystem* const OnlineSub = Online::GetSubsystem(GetWorld());
	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			if (SearchResult.IsValid()) {
				Sessions->AddOnJoinSessionCompleteDelegate_Handle(FOnJoinSessionCompleteDelegate::CreateUObject(this, &AEOSPlayerController::OnJoinSessionCompleteDelegate));

				TSharedPtr<const FUniqueNetId> UniqueNetIdptr = GetLocalPlayer()->GetPreferredUniqueNetId().GetUniqueNetId();
				Sessions->JoinSession(*UniqueNetIdptr, SESSION_NAME, SearchResult);
				UE_LOG_ONLINE(Log, TEXT("Join Session"), "");
			}
			else {
				UE_LOG_ONLINE(Log, TEXT("Invalid session."), "");
			}
		}
	}
}

void AEOSPlayerController::OnJoinSessionCompleteDelegate(FName SessionName, EOnJoinSessionCompleteResult::Type Result) {
	IOnlineSubsystem* const OnlineSub = Online::GetSubsystem(GetWorld());
	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			if (Result == EOnJoinSessionCompleteResult::Success)
			{
				// Client travel to the server
				FString ConnectString;
				if (Sessions->GetResolvedConnectString(SESSION_NAME, ConnectString))
				{
					UE_LOG_ONLINE_SESSION(Log, TEXT("Join session: traveling to %s"), *ConnectString);
					AEOSPlayerController::ClientTravel(ConnectString, TRAVEL_Absolute);
				}
			}
		}
	}
}

void AEOSPlayerController::KillSession()
{
	IOnlineSubsystem* const OnlineSub = Online::GetSubsystem(GetWorld());
	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			Sessions->DestroySession(SESSION_NAME);
			UGameplayStatics::OpenLevel(this, FName(TEXT("/Game/FirstPerson/Maps/Main")), true, "");
		}
	}
}
