struct TextureData {
	unsigned char* pixels;
	int width;
	int height;
};

struct Sprite {
	TextureData data;
	int x;
	int y;
};

struct SpriteSheet {
	std::vector<Sprite> sprites{};
	std::vector<uint8_t> pixels{};
	int texture_width;
	int texture_height;

	struct Slot {
		Slot() = default;
		Slot(int x, int y, int w, int h) noexcept : x(x), y(y), w(w), h(h) {}

		bool canFit(int width, int height) {
			return width <= w && height <= h;
		}

		Slot *find(int width, int height) {
			if (canFit(width, height)) {
				if (!ignore) {
					return this;
				}

				return findEmptySlot(subSlots, width, height);
			}

			return nullptr;
		}

		std::vector<std::unique_ptr<Slot>> subSlots;
		bool ignore = false;
		int x = 0;
		int y = 0;
		int w = 0;
		int h = 0;
	};

	bool PackTextures(std::span<TextureData> textures) {
		std::vector<std::unique_ptr<Slot>> slots;

		pixels.clear();
		sprites.clear();
		sprites.reserve(textures.size());

		int textureSize = std::max(
			nextPowerOf2(textures[0].width),
			nextPowerOf2(textures[0].height)
		);
		texture_width = textureSize;
		texture_height = textureSize;

		slots.emplace_back(new Slot(0, 0, textureSize, textureSize));
		for (auto const& data : textures) {
			auto slot = findEmptySlot(slots, data.width, data.height);
			if (slot == nullptr) {
				textureSize = std::max(
					nextPowerOf2(texture_width + data.width),
					nextPowerOf2(texture_height + data.height)
				);

				slots.reserve(2);
				slots.emplace_back(new Slot(texture_width, 0, textureSize - texture_width, texture_height));
				if (slots.back()->canFit(data.width, data.height)) {
					slot = slots.back().get();
				}
				slots.emplace_back(new Slot(0, texture_height, textureSize, textureSize - texture_height));
				if (!slot && slots.back()->canFit(data.width, data.height)) {
					slot = slots.back().get();
				}

				texture_width = textureSize;
				texture_height = textureSize;
			}

			if (slot != nullptr) {
				slot->ignore = true;

				if (slot->w != data.width)
					slot->subSlots.emplace_back(new Slot(slot->x + data.width, slot->y, slot->w - data.width, data.height));

				if (slot->h != data.height)
					slot->subSlots.emplace_back(new Slot(slot->x, slot->y + data.height, slot->w, slot->h - data.height));

				sprites.push_back(Sprite{
					.data = data,
					.x = slot->x,
					.y = slot->y
				});
			} else {
				return false;
			}
		}

		pixels.resize(texture_width * texture_height * 4);
		for (auto& sprite : sprites) {
			auto x = sprite.x;
			auto y = sprite.y;
			auto w = sprite.data.width;
			auto h = sprite.data.height;

			int i2 = 0;
			for (int iy = y; iy < y + h; iy++) {
				for (int ix = x; ix < x + w; ix++) {
					auto i = (iy * texture_width + ix) * 4;

					pixels[i++] = sprite.data.pixels[i2++];
					pixels[i++] = sprite.data.pixels[i2++];
					pixels[i++] = sprite.data.pixels[i2++];
					pixels[i++] = sprite.data.pixels[i2++];
				}
			}
		}

		return true;
	}

	inline static Slot* findEmptySlot(std::span<std::unique_ptr<Slot>> slots, int w, int h) {
		for (auto&& slot_ref : slots) {
			if (auto slot = slot_ref->find(w, h)) {
				return slot;
			}
		}
		return nullptr;
	}

	inline static constexpr unsigned nextPowerOf2(unsigned n) noexcept {
		n--;
		n |= n >> 1u;
		n |= n >> 2u;
		n |= n >> 4u;
		n |= n >> 8u;
		n |= n >> 16u;
		n++;
		return n;
	}
};
