// Fill out your copyright notice in the Description page of Project Settings.

#include "Pathfinding/Management/PathRequest.h"
#include "Pathfinding/Management/PathRequestManager.h"

FPathRequest::FPathRequest(const FVector& PathStart, const FVector& PathEnd, FPathRequestCallback InCallback,
	UPathRequestManager* InRequester): PathStart(PathStart),
	                                   PathEnd(PathEnd),
	                                   Callback(MoveTemp(InCallback)),
	                                   Requester(InRequester)
{}
