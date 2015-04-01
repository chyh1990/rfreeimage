module RFreeImage
	module ImageBPP
		GRAY = 8
		BGR = 24
		BGRA = 32
	end
	class Image
		def bytes
			@bytes ||= read_bytes
		end

		def destroy!
			release
			@bytes = nil
		end

		def gray?
			bpp == ImageBPP::GRAY
		end

		def to_gray
			return self if gray?
			to_bpp 8
		end
	end
end
