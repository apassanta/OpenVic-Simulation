#include "Date.hpp"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <charconv>

#include "openvic-simulation/utility/Logger.hpp"
#include "openvic-simulation/utility/StringUtils.hpp"

using namespace OpenVic;

Timespan::Timespan(day_t value) : days { value } {}

bool Timespan::operator<(Timespan other) const {
	return days < other.days;
};
bool Timespan::operator>(Timespan other) const {
	return days > other.days;
};
bool Timespan::operator<=(Timespan other) const {
	return days <= other.days;
};
bool Timespan::operator>=(Timespan other) const {
	return days >= other.days;
};
bool Timespan::operator==(Timespan other) const {
	return days == other.days;
};
bool Timespan::operator!=(Timespan other) const {
	return days != other.days;
};

Timespan Timespan::operator+(Timespan other) const {
	return days + other.days;
}

Timespan Timespan::operator-(Timespan other) const {
	return days - other.days;
}

Timespan Timespan::operator*(day_t factor) const {
	return days * factor;
}

Timespan Timespan::operator/(day_t factor) const {
	return days / factor;
}

Timespan& Timespan::operator+=(Timespan other) {
	days += other.days;
	return *this;
}

Timespan& Timespan::operator-=(Timespan other) {
	days -= other.days;
	return *this;
}

Timespan& Timespan::operator++() {
	days++;
	return *this;
}

Timespan Timespan::operator++(int) {
	Timespan old = *this;
	++(*this);
	return old;
}

Timespan::operator day_t() const {
	return days;
}

Timespan::operator double() const {
	return days;
}

std::string Timespan::to_string() const {
	return std::to_string(days);
}

Timespan::operator std::string() const {
	return to_string();
}

Timespan Timespan::from_years(day_t num) {
	return num * Date::DAYS_IN_YEAR;
}

Timespan Timespan::from_months(day_t num) {
	return (num / Date::MONTHS_IN_YEAR) * Date::DAYS_IN_YEAR + Date::DAYS_UP_TO_MONTH[num % Date::MONTHS_IN_YEAR];
}

Timespan Timespan::from_days(day_t num) {
	return num;
}

std::ostream& OpenVic::operator<<(std::ostream& out, Timespan const& timespan) {
	return out << timespan.to_string();
}

Timespan Date::_date_to_timespan(year_t year, month_t month, day_t day) {
	month = std::clamp<month_t>(month, 1, MONTHS_IN_YEAR);
	day = std::clamp<day_t>(day, 1, DAYS_IN_MONTH[month - 1]);
	return year * DAYS_IN_YEAR + DAYS_UP_TO_MONTH[month - 1] + day - 1;
}

Timespan::day_t const* Date::DAYS_UP_TO_MONTH = generate_days_up_to_month();

Timespan::day_t const* Date::generate_days_up_to_month() {
	static Timespan::day_t days_up_to_month[MONTHS_IN_YEAR];
	Timespan::day_t days = 0;
	int month = 0;
	while (month < MONTHS_IN_YEAR) {
		days_up_to_month[month] = days;
		days += DAYS_IN_MONTH[month++];
	}
	assert(days == DAYS_IN_YEAR);
	return days_up_to_month;
}

Date::month_t const* Date::MONTH_FROM_DAY_IN_YEAR = generate_month_from_day_in_year();

Date::month_t const* Date::generate_month_from_day_in_year() {
	static month_t month_from_day_in_year[DAYS_IN_YEAR];
	Timespan::day_t days_left = 0;
	int day = 0, month = 0;
	while (day < DAYS_IN_YEAR) {
		days_left = (days_left > 0 ? days_left : DAYS_IN_MONTH[month++]) - 1;
		month_from_day_in_year[day++] = month;
	}
	assert(days_left == 0);
	assert(month_from_day_in_year[DAYS_IN_YEAR - 1] == MONTHS_IN_YEAR);
	return month_from_day_in_year;
}

Date::Date(Timespan total_days) : timespan { total_days } {
	if (timespan < 0) {
		Logger::error("Invalid timespan for date: ", timespan, " (cannot be negative)");
		timespan = 0;
	}
}

Date::Date(year_t year, month_t month, day_t day) : timespan { _date_to_timespan(year, month, day) } {}

Date::year_t Date::get_year() const {
	return static_cast<Timespan::day_t>(timespan) / DAYS_IN_YEAR;
}

Date::month_t Date::get_month() const {
	return MONTH_FROM_DAY_IN_YEAR[static_cast<Timespan::day_t>(timespan) % DAYS_IN_YEAR];
}

Date::day_t Date::get_day() const {
	return (static_cast<Timespan::day_t>(timespan) % DAYS_IN_YEAR) - DAYS_UP_TO_MONTH[get_month() - 1] + 1;
}

bool Date::operator<(Date other) const {
	return timespan < other.timespan;
};
bool Date::operator>(Date other) const {
	return timespan > other.timespan;
};
bool Date::operator<=(Date other) const {
	return timespan <= other.timespan;
};
bool Date::operator>=(Date other) const {
	return timespan >= other.timespan;
};
bool Date::operator==(Date other) const {
	return timespan == other.timespan;
};
bool Date::operator!=(Date other) const {
	return timespan != other.timespan;
};

Date Date::operator+(Timespan other) const {
	return timespan + other;
}

Timespan Date::operator-(Date other) const {
	return timespan - other.timespan;
}

Date& Date::operator+=(Timespan other) {
	timespan += other;
	return *this;
}

Date& Date::operator-=(Timespan other) {
	timespan -= other;
	return *this;
}

Date& Date::operator++() {
	timespan++;
	return *this;
}

Date Date::operator++(int) {
	Date old = *this;
	++(*this);
	return old;
}

bool Date::in_range(Date start, Date end) const {
	return start <= *this && *this <= end;
}

std::string Date::to_string() const {
	std::stringstream ss;
	ss << *this;
	return ss.str();
}

Date::operator std::string() const {
	return to_string();
}

std::ostream& OpenVic::operator<<(std::ostream& out, Date date) {
	return out << static_cast<int>(date.get_year()) << Date::SEPARATOR_CHARACTER << static_cast<int>(date.get_month())
		<< Date::SEPARATOR_CHARACTER << static_cast<int>(date.get_day());
}

// Parsed from string of the form YYYY.MM.DD
Date Date::from_string(char const* const str, char const* const end, bool* successful, bool quiet) {
	if (successful != nullptr) {
		*successful = true;
	}

	year_t year = 0;
	month_t month = 1;
	day_t day = 1;

	if (str == nullptr || end <= str) {
		if (!quiet) {
			Logger::error(
				"Invalid string start/end pointers: ", static_cast<void const*>(str), " - ", static_cast<void const*>(end)
			);
		}
		if (successful != nullptr) {
			*successful = false;
		}
		return { year, month, day };
	}

	char const* year_end = str;
	while (std::isdigit(*year_end) && ++year_end < end) {}

	if (year_end <= str) {
		if (!quiet) {
			Logger::error("Failed to find year digits in date: ", std::string_view { str, static_cast<size_t>(end - str) });
		}
		if (successful != nullptr) {
			*successful = false;
		}
		return { year, month, day };
	}

	bool sub_successful = false;
	uint64_t val = StringUtils::string_to_uint64(str, year_end, &sub_successful, 10);
	if (!sub_successful || val > std::numeric_limits<year_t>::max()) {
		if (!quiet) {
			Logger::error("Failed to read year: ", std::string_view { str, static_cast<size_t>(end - str) });
		}
		if (successful != nullptr) {
			*successful = false;
		}
		return { year, month, day };
	}
	year = val;
	if (year_end < end) {
		if (*year_end == SEPARATOR_CHARACTER) {
			char const* const month_start = year_end + 1;
			char const* month_end = month_start;
			if (month_start < end) {
				while (std::isdigit(*month_end) && ++month_end < end) {}
			}
			if (month_start >= month_end) {
				if (!quiet) {
					Logger::error(
						"Failed to find month digits in date: ", std::string_view { str, static_cast<size_t>(end - str) }
					);
				}
				if (successful != nullptr) {
					*successful = false;
				}
			} else {
				sub_successful = false;
				val = StringUtils::string_to_uint64(month_start, month_end, &sub_successful, 10);
				if (!sub_successful || val < 1 || val > MONTHS_IN_YEAR) {
					if (!quiet) {
						Logger::error("Failed to read month: ", std::string_view { str, static_cast<size_t>(end - str) });
					}
					if (successful != nullptr) {
						*successful = false;
					}
				} else {
					month = val;
					if (month_end < end) {
						if (*month_end == SEPARATOR_CHARACTER) {
							char const* const day_start = month_end + 1;
							char const* day_end = day_start;
							if (day_start < end) {
								while (std::isdigit(*day_end) && ++day_end < end) {}
							}
							if (day_start >= day_end) {
								if (!quiet) {
									Logger::error(
										"Failed to find day digits in date: ",
										std::string_view { str, static_cast<size_t>(end - str) }
									);
								}
								if (successful != nullptr) {
									*successful = false;
								}
							} else {
								sub_successful = false;
								val = StringUtils::string_to_uint64(day_start, day_end, &sub_successful);
								if (!sub_successful || val < 1 || val > DAYS_IN_MONTH[month - 1]) {
									if (!quiet) {
										Logger::error(
											"Failed to read day: ", std::string_view { str, static_cast<size_t>(end - str) }
										);
									}
									if (successful != nullptr) {
										*successful = false;
									}
								} else {
									day = val;
									if (day_end < end) {
										if (!quiet) {
											Logger::error(
												"Unexpected string \"",
												std::string_view { day_end, static_cast<size_t>(end - day_end) },
												"\" at the end of date ",
												std::string_view { str, static_cast<size_t>(end - str) }
											);
										}
										if (successful != nullptr) {
											*successful = false;
										}
									}
								}
							}
						} else {
							if (!quiet) {
								Logger::error(
									"Unexpected character \"", *month_end, "\" in month of date ",
									std::string_view { str, static_cast<size_t>(end - str) }
								);
							}
							if (successful != nullptr) {
								*successful = false;
							}
						}
					}
				}
			}
		} else {
			if (!quiet) {
				Logger::error(
					"Unexpected character \"", *year_end, "\" in year of date ",
					std::string_view { str, static_cast<size_t>(end - str) }
				);
			}
			if (successful != nullptr) {
				*successful = false;
			}
		}
	}
	return { year, month, day };
};

Date Date::from_string(char const* str, size_t length, bool* successful, bool quiet) {
	return from_string(str, str + length, successful, quiet);
}

Date Date::from_string(std::string_view str, bool* successful, bool quiet) {
	return from_string(str.data(), str.length(), successful, quiet);
}
