//
// Created by gabe on 8/24/25.
//

#ifndef CPP_ENGINE_IMGUILOGSINK_H
#define CPP_ENGINE_IMGUILOGSINK_H

#include <spdlog/sinks/base_sink.h>
#include <mutex>
#include <deque>
#include <string>

class ImGuiLogSink : public spdlog::sinks::base_sink<std::mutex> {
  public:
	struct LogMessage {
		spdlog::level::level_enum level;
		std::string               message;
	};

	void clear()
	{
		std::lock_guard<std::mutex> lock(mutex_);
		messages_.clear();
	}

	const std::deque<LogMessage>& messages() const { return messages_; }

  protected:
	void sink_it_(const spdlog::details::log_msg& msg) override
	{
		// Format message to string
		spdlog::memory_buf_t formatted;
		base_sink<std::mutex>::formatter_->format(msg, formatted);

		std::lock_guard<std::mutex> lock(mutex_);
		messages_.push_back({msg.level, fmt::to_string(formatted)});

		// Limit stored messages to prevent unbounded growth
		if (messages_.size() > max_messages_) messages_.pop_front();
	}

	void flush_() override {}

  private:
	std::deque<LogMessage> messages_;
	const size_t           max_messages_ = 2000;
	std::mutex             mutex_;
};


#endif // CPP_ENGINE_IMGUILOGSINK_H
