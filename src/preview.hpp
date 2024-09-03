#pragma once

#include <memory>

#include <SFML/Graphics.hpp>

struct Preview
{
	sf::RenderTexture previewTexture;
	sf::RenderTexture previewFullscreenTexture;

	sf::View previewView;
	float zoom = 1.f;

	bool isFullScreenMode{};

	bool isDragging = false;

	enum class ShapeKind
	{
		Square,
		Circle,
		Triangle,
		Hexagon,
		Star,
	};

	struct ShapeSettings
	{
		ShapeKind kind{};
		float size = 100.f;
		sf::Color color = sf::Color::White;
	};

	ShapeSettings shapeSettings;
	std::unique_ptr<sf::Drawable> shape;

	auto makeRectangleShape()
	{
		auto rect = std::make_unique<sf::RectangleShape>(sf::Vector2f{ shapeSettings.size, shapeSettings.size });
		rect->setFillColor(shapeSettings.color);
		rect->setOrigin({ shapeSettings.size / 2.f, shapeSettings.size / 2.f });
		return rect;
	}

	auto makeCircleShape()
	{
		auto circle = std::make_unique<sf::CircleShape>(shapeSettings.size / 2.f);
		circle->setFillColor(shapeSettings.color);
		circle->setOrigin({ shapeSettings.size / 2.f, shapeSettings.size / 2.f });
		return circle;
	}

	auto makeTriangleShape()
	{
		std::unique_ptr<sf::ConvexShape> ptr = std::make_unique<sf::ConvexShape>(3);
		ptr->setFillColor(shapeSettings.color);
		ptr->setOrigin({ shapeSettings.size / 2.f, shapeSettings.size / 2.f });
		
		ptr->setPoint(0, { shapeSettings.size / 2.f, 0.f });
		ptr->setPoint(1, { shapeSettings.size, shapeSettings.size });
		ptr->setPoint(2, { 0.f, shapeSettings.size });

		return ptr;
	}

	auto makeHexagonShape()
	{
		auto hex = std::make_unique<sf::CircleShape>(shapeSettings.size / 2.f, 6);
		hex->setFillColor(shapeSettings.color);
		hex->setOrigin({ shapeSettings.size / 2.f, shapeSettings.size / 2.f });
		hex->rotate(sf::degrees(360.f / 6.f / 2.f));
		return hex;
	}

	auto makeStarShape()
	{
		std::unique_ptr<sf::ConvexShape> ptr = std::make_unique<sf::ConvexShape>(10);
		ptr->setFillColor(shapeSettings.color);

		for (int x = 0; x < 10; x++)
		{
			ptr->setPoint(x, sf::Vector2f{ 0.f, -shapeSettings.size / (x % 2 == 0 ? 2.f : 4.f) }.rotatedBy(sf::degrees(360.f / 10.f * x)));
		}

		return ptr;
	}

	void updateShape()
	{
		if (shapeSettings.kind == ShapeKind::Square)
		{
			shape = makeRectangleShape();
		}
		else if (shapeSettings.kind == ShapeKind::Circle)
		{
			shape = makeCircleShape();
		}
		else if (shapeSettings.kind == ShapeKind::Triangle)
		{
			shape = makeTriangleShape();
		}
		else if (shapeSettings.kind == ShapeKind::Hexagon)
		{
			shape = makeHexagonShape();
		}
		else if (shapeSettings.kind == ShapeKind::Star)
		{
			shape = makeStarShape();
		}
	}

	Preview()
	{
		updateShape();
	}

	void update(sf::Shader& shader)
	{
		ImGui::Checkbox("Fullscreen", &isFullScreenMode);
		ImGui::SameLine();

		ImGui::Dummy({ 10, 0 });
		ImGui::SameLine();
		
		const auto showButton = [&](auto name, auto value)
		{
			ImGui::PushID(name);
			if (ImGui::Selectable(name, shapeSettings.kind == value, 0, ImGui::CalcTextSize(name, NULL, true)))
			{
				shapeSettings.kind = value;
				updateShape();
			}
			ImGui::PopID();
		};

		showButton("Square", ShapeKind::Square);
		ImGui::SameLine();
		showButton("Circle", ShapeKind::Circle);
		ImGui::SameLine();
		showButton("Triangle", ShapeKind::Triangle);
		ImGui::SameLine();
		showButton("Hexagon", ShapeKind::Hexagon);
		ImGui::SameLine();
		showButton("Star", ShapeKind::Star);

		ImVec2 contentSize{};

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
		ImGui::BeginChild("preview_child");
		// Yellow is content region min/max
		{
			auto vMin = ImGui::GetWindowContentRegionMin();
			auto vMax = ImGui::GetWindowContentRegionMax();

			contentSize = vMax - vMin - sf::Vector2f{ 2.f, 2.f };

			if (contentSize.x != previewTexture.getSize().x || contentSize.y != previewTexture.getSize().y)
			{
				previewView = sf::View({}, contentSize);
				zoom = 1.f;

				(void)previewTexture.resize(sf::Vector2u(contentSize.x, contentSize.y));
			}

			previewTexture.clear();
			auto renderView = previewView;
			renderView.zoom(zoom);
			previewTexture.setView(renderView);

			sf::RenderStates states;
			if (!isFullScreenMode)
			{
				states.shader = &shader;
			}

			previewTexture.draw(*shape);

			previewTexture.display();

			if (isFullScreenMode)
			{
				if (previewFullscreenTexture.getSize() != previewTexture.getSize())
				{
					(void)previewFullscreenTexture.resize(sf::Vector2u(contentSize.x, contentSize.y));
				}

				sf::RectangleShape rect(contentSize);
				rect.setFillColor(sf::Color::White);
				rect.setTexture(&previewTexture.getTexture(), true);
				sf::RenderStates states;

				shader.setUniform("texture", previewTexture.getTexture());
				states.shader = &shader;

				previewFullscreenTexture.setView({ contentSize / 2, contentSize });
				previewFullscreenTexture.draw(rect, states);
				previewFullscreenTexture.display();

				ImGui::Image(previewFullscreenTexture, contentSize, sf::Color::White, sf::Color::Yellow);
			}
			else
			{
				ImGui::Image(previewTexture, contentSize, sf::Color::White, sf::Color::Yellow);
			}
		}

		if (ImGui::IsItemHovered())
		{
			const auto scroll = ImGui::GetIO().MouseWheel;
			if (scroll > 0)
			{
				zoom *= 0.9;
			}
			else if (scroll < 0)
			{
				zoom *= 1.1;
			}
			
		}

		if (ImGui::IsItemClicked())
		{
			isDragging = true;

			if (ImGui::GetMouseClickedCount(0) == 2)
			{
				previewView = sf::View({}, contentSize);
				zoom = 1.f;
			}
		}

		if (!ImGui::IsMouseDown(0))
		{
			isDragging = false;
		}

		if (isDragging)
		{
			previewView.move(-ImGui::GetIO().MouseDelta * zoom);
		}

		auto center = previewView.getCenter();
		const auto halfSize = contentSize / 2.f;
		center.x = std::clamp(center.x, -halfSize.x, halfSize.x);
		center.y = std::clamp(center.y, -halfSize.y, halfSize.y);
		previewView.setCenter(center);

		zoom = std::clamp(zoom, 0.4f, 3.f);

		ImGui::EndChild();
		ImGui::PopStyleVar();
	}
};