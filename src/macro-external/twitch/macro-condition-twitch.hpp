#pragma once
#include "macro-condition-edit.hpp"
#include "token.hpp"
#include "category-selection.hpp"
#include "channel-selection.hpp"

#include <variable-line-edit.hpp>
#include <regex-config.hpp>

namespace advss {

class MacroConditionTwitch : public MacroCondition {
public:
	MacroConditionTwitch(Macro *m) : MacroCondition(m, true) {}
	bool CheckCondition();
	bool Save(obs_data_t *obj) const;
	bool Load(obs_data_t *obj);
	std::string GetShortDesc() const;
	std::string GetId() const { return id; };
	static std::shared_ptr<MacroCondition> Create(Macro *m)
	{
		return std::make_shared<MacroConditionTwitch>(m);
	}
	bool ConditionIsSupportedByToken();

	enum class Condition {
		// Generic event based
		STREAM_ONLINE_EVENT = 0,
		STREAM_OFFLINE_EVENT = 100,
		CHANNEL_INFO_UPDATE_EVENT = 200,
		FOLLOW_EVENT = 300,
		SUBSCRIPTION_START_EVENT = 400,
		SUBSCRIPTION_END_EVENT = 500,
		SUBSCRIPTION_GIFT_EVENT = 600,
		SUBSCRIPTION_MESSAGE_EVENT = 700,
		CHEER_EVENT = 800,
		RAID_OUTBOUND_EVENT = 900,
		RAID_INBOUND_EVENT = 1000,
		SHOUTOUT_OUTBOUND_EVENT = 1100,
		SHOUTOUT_INBOUND_EVENT = 1200,
		POLL_START_EVENT = 1300,
		POLL_PROGRESS_EVENT = 1400,
		POLL_END_EVENT = 1500,
		PREDICTION_START_EVENT = 1600,
		PREDICTION_PROGRESS_EVENT = 1700,
		PREDICTION_LOCK_EVENT = 1800,
		PREDICTION_END_EVENT = 1900,
		GOAL_START_EVENT = 2000,
		GOAL_PROGRESS_EVENT = 2100,
		GOAL_END_EVENT = 2200,
		HYPE_TRAIN_START_EVENT = 2300,
		HYPE_TRAIN_PROGRESS_EVENT = 2400,
		HYPE_TRAIN_END_EVENT = 2500,
		CHARITY_CAMPAIGN_START_EVENT = 2600,
		CHARITY_CAMPAIGN_PROGRESS_EVENT = 2700,
		CHARITY_CAMPAIGN_DONATION_EVENT = 2800,
		CHARITY_CAMPAIGN_END_EVENT = 2900,
		SHIELD_MODE_START_EVENT = 3000,
		SHIELD_MODE_END_EVENT = 3100,
		POINTS_REWARD_ADDITION_EVENT = 3200,
		POINTS_REWARD_UPDATE_EVENT = 3300,
		POINTS_REWARD_DELETION_EVENT = 3400,
		POINTS_REWARD_REDEMPTION_EVENT = 3500,
		POINTS_REWARD_REDEMPTION_UPDATE_EVENT = 3600,
		USER_BAN_EVENT = 3700,
		USER_UNBAN_EVENT = 3800,
		USER_MODERATOR_ADDITION_EVENT = 3900,
		USER_MODERATOR_DELETION_EVENT = 4000,

		// Event based with custom logic
		STREAM_ONLINE_LIVE_EVENT = 1,
		STREAM_ONLINE_PLAYLIST_EVENT = 2,
		STREAM_ONLINE_WATCHPARTY_EVENT = 3,
		STREAM_ONLINE_PREMIERE_EVENT = 4,
		STREAM_ONLINE_RERUN_EVENT = 5,

		// Polling
		LIVE_POLLING = 1000000,
		TITLE_POLLING = 1000100,
		CATEGORY_POLLING = 1000200,
	};

	void SetCondition(const Condition &);
	Condition GetCondition() { return _condition; }

	std::weak_ptr<TwitchToken> _token;
	TwitchChannel _channel;
	StringVariable _streamTitle = obs_module_text(
		"AdvSceneSwitcher.condition.twitch.title.title");
	RegexConfig _regex = RegexConfig::PartialMatchRegexConfig();
	TwitchCategory _category;

private:
	bool CheckChannelGenericEvents(
		TwitchToken &,
		const char *mainUserIdFieldName = "broadcaster_user_id");
	bool CheckChannelLiveEvents(TwitchToken &);

	bool IsUsingEventSubCondition();
	void SetupEventSubscriptions();
	void CheckEventSubscription(EventSub &);
	void AddChannelGenericEventSubscription(
		const char *version, bool includeModeratorId = false,
		const char *mainUserIdFieldName = "broadcaster_user_id");

	Condition _condition = Condition::LIVE_POLLING;
	std::future<std::string> _subscriptionIDFuture;
	std::string _subscriptionID;
	static bool _registered;
	static const std::string id;
};

class MacroConditionTwitchEdit : public QWidget {
	Q_OBJECT

public:
	MacroConditionTwitchEdit(
		QWidget *parent,
		std::shared_ptr<MacroConditionTwitch> entryData = nullptr);
	void UpdateEntryData();
	static QWidget *Create(QWidget *parent,
			       std::shared_ptr<MacroCondition> cond)
	{
		return new MacroConditionTwitchEdit(
			parent,
			std::dynamic_pointer_cast<MacroConditionTwitch>(cond));
	}

private slots:
	void ConditionChanged(int);
	void TwitchTokenChanged(const QString &);
	void StreamTitleChanged();
	void CategoreyChanged(const TwitchCategory &);
	void ChannelChanged(const TwitchChannel &);
	void RegexChanged(RegexConfig);
	void CheckTokenPermissions();

signals:
	void HeaderInfoChanged(const QString &);

protected:
	std::shared_ptr<MacroConditionTwitch> _entryData;

private:
	void SetupWidgetVisibility();

	FilterComboBox *_conditions;
	TwitchConnectionSelection *_tokens;
	VariableLineEdit *_streamTitle;
	TwitchCategoryWidget *_category;
	TwitchChannelSelection *_channel;
	RegexConfigWidget *_regex;
	QHBoxLayout *_layout;
	QLabel *_tokenPermissionWarning;
	QTimer _tokenPermissionCheckTimer;
	bool _loading = true;
};

} // namespace advss