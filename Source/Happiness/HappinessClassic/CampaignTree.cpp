#include "CampaignTree.h"

#define LOCTEXT_NAMESPACE "CampaignTree"

const TArray<TArray<ECampaignLesson>>& UCampaignTree::GetColumns()
{
	static const TArray<TArray<ECampaignLesson>> Columns =
	{
		{ ECampaignLesson::VerticalTwo },	// Game Basics
		{ ECampaignLesson::VerticalThree },
		{ ECampaignLesson::NextTo, ECampaignLesson::DirectlyLeftOf },
		{ ECampaignLesson::Span },
		{ ECampaignLesson::Edge },
		{ ECampaignLesson::TwoNot, ECampaignLesson::ThreeNot, ECampaignLesson::NotNextTo },
		{ ECampaignLesson::SpanNotSide, ECampaignLesson::Gap, ECampaignLesson::SpanNotMid },
		{ ECampaignLesson::LeftOf, ECampaignLesson::NotLeftOf },
		{ ECampaignLesson::Between },
		{ ECampaignLesson::Chain },
		{ ECampaignLesson::AllApart },
		{ ECampaignLesson::EitherOr, ECampaignLesson::NextToEitherOr },
	};
	return Columns;
}

int32 UCampaignTree::GetNumColumns()
{
	return GetColumns().Num();
}

TArray<ECampaignLesson> UCampaignTree::GetColumnLessons(int32 Column)
{
	return GetColumns().IsValidIndex(Column) ? GetColumns()[Column] : TArray<ECampaignLesson>();
}

int32 UCampaignTree::GetLessonColumn(ECampaignLesson Lesson)
{
	const TArray<TArray<ECampaignLesson>>& Columns = GetColumns();
	for (int32 Column = 0; Column < Columns.Num(); Column++)
	{
		if (Columns[Column].Contains(Lesson))
		{
			return Column;
		}
	}
	return INDEX_NONE;
}

FText UCampaignTree::GetLessonDisplayName(ECampaignLesson Lesson)
{
	switch (Lesson)
	{
		case ECampaignLesson::VerticalTwo:		return LOCTEXT("VerticalTwo", "Game Basics");
		case ECampaignLesson::VerticalThree:	return LOCTEXT("VerticalThree", "Three in a Column");
		case ECampaignLesson::NextTo:			return LOCTEXT("NextTo", "Next To");
		case ECampaignLesson::DirectlyLeftOf:	return LOCTEXT("DirectlyLeftOf", "Directly Left Of");
		case ECampaignLesson::Span:				return LOCTEXT("Span", "Span");
		case ECampaignLesson::Edge:				return LOCTEXT("Edge", "Edges");
		case ECampaignLesson::TwoNot:			return LOCTEXT("TwoNot", "Not Same Column");
		case ECampaignLesson::ThreeNot:			return LOCTEXT("ThreeNot", "Two Together, One Not");
		case ECampaignLesson::NotNextTo:		return LOCTEXT("NotNextTo", "Not Next To");
		case ECampaignLesson::SpanNotSide:		return LOCTEXT("SpanNotSide", "Span With a Not");
		case ECampaignLesson::Gap:				return LOCTEXT("Gap", "Gap");
		case ECampaignLesson::SpanNotMid:		return LOCTEXT("SpanNotMid", "Gap Without");
		case ECampaignLesson::LeftOf:			return LOCTEXT("LeftOf", "Left Of");
		case ECampaignLesson::NotLeftOf:		return LOCTEXT("NotLeftOf", "Not Left Of");
		case ECampaignLesson::Between:			return LOCTEXT("Between", "Between");
		case ECampaignLesson::Chain:			return LOCTEXT("Chain", "Chain");
		case ECampaignLesson::AllApart:			return LOCTEXT("AllApart", "All Apart");
		case ECampaignLesson::EitherOr:			return LOCTEXT("EitherOr", "Either Or");
		case ECampaignLesson::NextToEitherOr:	return LOCTEXT("NextToEitherOr", "Next To Either");
		default:								return FText::GetEmpty();
	}
}

FText UCampaignTree::GetLessonDescription(ECampaignLesson Lesson)
{
	switch (Lesson)
	{
		case ECampaignLesson::VerticalTwo:
			return LOCTEXT("VerticalTwoDesc", "Every row holds each of its icons exactly once. Some icons start already placed. Pick an icon for a cell, or rule icons out, until every cell is filled.\n\nThe \"same column\" clue shows two icons stacked: wherever one of them is, the other is in the same column.");
		case ECampaignLesson::VerticalThree:
			return LOCTEXT("VerticalThreeDesc", "Three icons stacked means all three are in the same column. Find where any one of them goes and you know where the other two go.");
		case ECampaignLesson::NextTo:
			return LOCTEXT("NextToDesc", "Two icons side by side are next to each other: directly left or directly right. An icon on the edge only has one neighbor, which can settle it.");
		case ECampaignLesson::DirectlyLeftOf:
			return LOCTEXT("DirectlyLeftOfDesc", "Joined by a bracket, the first icon is directly to the left of the second. Unlike Next To, the order is fixed.");
		case ECampaignLesson::Span:
			return LOCTEXT("SpanDesc", "Three icons in a row under an arrow: the middle one is between the other two, each directly beside it. The outer two can be either way round.");
		case ECampaignLesson::Edge:
			return LOCTEXT("EdgeDesc", "An icon marked as on the edge is in the first or last column. One marked as not on the edge is never in either end column.");
		case ECampaignLesson::TwoNot:
			return LOCTEXT("TwoNotDesc", "A crossed-out pair means the two icons are never in the same column. Where you find one, rule the other out of that column.");
		case ECampaignLesson::ThreeNot:
			return LOCTEXT("ThreeNotDesc", "Three stacked icons with one crossed out: the other two share a column, and the crossed-out icon is not in it.");
		case ECampaignLesson::NotNextTo:
			return LOCTEXT("NotNextToDesc", "A crossed-out pair side by side means the two icons are never next to each other.");
		case ECampaignLesson::SpanNotSide:
			return LOCTEXT("SpanNotSideDesc", "The middle icon has one icon directly beside it on one side, and the crossed-out icon is not beside it on the other side.");
		case ECampaignLesson::Gap:
			return LOCTEXT("GapDesc", "Two icons with an empty box between them have exactly one column between them. They can be either way round, and anything could be in the column between them.");
		case ECampaignLesson::SpanNotMid:
			return LOCTEXT("SpanNotMidDesc", "The two outer icons have exactly one column between them, either way round. The crossed-out middle icon is not in that column.");
		case ECampaignLesson::LeftOf:
			return LOCTEXT("LeftOfDesc", "The first icon is somewhere to the left of the second: not always right next to it. The left icon can't be in the last column, and the right one can't be in the first.");
		case ECampaignLesson::NotLeftOf:
			return LOCTEXT("NotLeftOfDesc", "The first icon is never anywhere to the left of the second. It is to the right, or in the same column.");
		case ECampaignLesson::Between:
			return LOCTEXT("BetweenDesc", "The middle icon is somewhere between the other two, with any number of columns in between. The outer two can be either way round.");
		case ECampaignLesson::Chain:
			return LOCTEXT("ChainDesc", "Left to right, in this order: the first icon is left of the second, which is left of the third. Gaps can be any size.");
		case ECampaignLesson::AllApart:
			return LOCTEXT("AllApartDesc", "The three icons are all in different columns. No two of them ever share a column.");
		case ECampaignLesson::EitherOr:
			return LOCTEXT("EitherOrDesc", "The top icon is in the column of one of the two icons below it, but not both. One of those two is a decoy.");
		case ECampaignLesson::NextToEitherOr:
			return LOCTEXT("NextToEitherOrDesc", "The middle icon is next to one of the two outer icons, but not both. One of them is a decoy.");
		default:
			return FText::GetEmpty();
	}
}

bool UCampaignTree::IsClueLessonAllowed(ECampaignLesson ClueLesson, ECampaignLesson CurrentLesson)
{
	if (ClueLesson == ECampaignLesson::Given || ClueLesson == CurrentLesson)
	{
		return true;
	}

	const int32 ClueColumn = GetLessonColumn(ClueLesson);
	const int32 CurrentColumn = GetLessonColumn(CurrentLesson);
	return ClueColumn != INDEX_NONE && CurrentColumn != INDEX_NONE && ClueColumn < CurrentColumn;
}

#undef LOCTEXT_NAMESPACE
