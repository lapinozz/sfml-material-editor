#pragma once

#include <string>

#include <nvdialog.h>

namespace Dialog
{
	void init()
	{
		const static auto _ = []() -> int
		{
			if (nvd_init() != 0)
			{
				puts("Failed to initialize NvDialog.\n");
				exit(EXIT_FAILURE);
			}

			return 0;
		}();
	}

	enum class Status
	{
		Ok = 0,
		Yes = Ok,
		No,
		Cancel,
		None
	};

	enum class Type
	{
		Error,
		YesNoCancel
	};

	Status Show(Type type, std::string title, std::string text)
	{
		init();

		if (type == Type::YesNoCancel)
		{
			NvdQuestionBox* dialog = nvd_dialog_question_new(
				title.c_str(),
				text.c_str(),
				NVD_YES_NO_CANCEL
			);

			const auto reply = nvd_get_reply(dialog);

			nvd_free_object(dialog);

			if (reply == NVD_REPLY_OK)
			{
				return Status::Yes;
			}
			else if (reply == NVD_REPLY_NO)
			{
				return Status::No;
			}
			else if (reply == NVD_REPLY_CANCEL)
			{
				return Status::Cancel;
			}

			assert("unknown reply type" && false);
		}
		else if (type == Type::Error)
		{
			NvdDialogBox* dialog = nvd_dialog_box_new(
				title.c_str(),
				text.c_str(),
				NVD_DIALOG_ERROR
			);

			nvd_show_dialog(dialog);

			nvd_free_object(dialog);

			return Status::None;
		}

		assert("Unsupported dialog type" && false);
	}
}