package main

import (
	"fmt"
	"image"
	"image/color"
	"image/png"
	_ "image/png"
	"os"
	"path/filepath"
	"sort"

	"github.com/anthonynsimon/bild/transform"
	"github.com/esimov/colorquant"
	"golang.org/x/image/bmp"
	"golang.org/x/image/draw"

	_ "golang.org/x/image/bmp"
)

func main() {

	fmt.Println("### Start Building Assets ###")

	fmt.Println("creating ship spritesheet")
	err := createShipSprites()
	if err != nil {
		fmt.Println("error creating ship sprites:", err)
	}
}

func createShipSprites() error {

	total_frames := 32

	canvas := image.NewRGBA(image.Rectangle{Max: image.Point{X: 32, Y: 32 * total_frames}})
	draw.Draw(canvas, canvas.Bounds(), &image.Uniform{color.RGBA{R: 0, G: 0, B: 0, A: 255}}, image.Point{}, draw.Over)
	canvas2x := image.NewRGBA(image.Rectangle{Max: image.Point{X: 64, Y: 64 * total_frames}})
	draw.Draw(canvas2x, canvas.Bounds(), &image.Uniform{color.RGBA{R: 0, G: 0, B: 0, A: 255}}, image.Point{}, draw.Over)

	for i := range total_frames {

		img, _, _, err := loadImage(fmt.Sprintf("drone_frames/%04d", i+1), 256, 256, 0)
		if err != nil {
			return err
		}

		scaled := transform.Resize(img, 32, 32, transform.NearestNeighbor)

		draw.Draw(
			canvas,
			image.Rect(0, i*32, 64, 32+i*32),
			scaled,
			image.Point{0, 0},
			draw.Over)

		scaled2x := transform.Resize(img, 64, 64, transform.NearestNeighbor)

		draw.Draw(
			canvas2x,
			image.Rect(0, i*64, 128, 64+i*64),
			scaled2x,
			image.Point{0, 0},
			draw.Over)
	}

	processed := reducedToPaletted(canvas, nil)
	processed2x := reducedToPaletted(canvas2x, nil)

	cwd, err := os.Getwd()
	if err != nil {
		return err
	}
	f, err := os.Create(filepath.Join(cwd, "../game/graphics/ship.bmp"))
	if err != nil {
		return err
	}
	if err := bmp.Encode(f, processed); err != nil {
		f.Close()
		return err
	}
	if err := f.Close(); err != nil {
		return err
	}

	f, err = os.Create(filepath.Join(cwd, "../game/graphics/ship2x.bmp"))
	if err != nil {
		return err
	}
	if err := bmp.Encode(f, processed2x); err != nil {
		f.Close()
		return err
	}
	if err := f.Close(); err != nil {
		return err
	}

	return nil
}

type ReductionOptions struct {
	colorCount       int
	clearColor       color.RGBA
	ditherIntensity  float32
	ditherAdd        float32
	ditherClearColor bool
}

func reducedToPaletted(img *image.RGBA, options *ReductionOptions) *image.Paletted {
	if options == nil {
		options = &ReductionOptions{
			ditherIntensity:  0.5,
			ditherAdd:        -16.0,
			ditherClearColor: false,
		}
	}
	if options.colorCount <= 0 {
		options.colorCount = 16
	}

	// copy the original image
	canvas := image.NewRGBA(img.Rect)
	draw.Draw(canvas, canvas.Bounds(), &image.Uniform{options.clearColor}, image.Point{}, draw.Over)
	draw.Draw(canvas, canvas.Bounds(), img, image.Point{}, draw.Over)

	colorquant.NoDither.Quantize(img, canvas, options.colorCount, false, true)

	colors := make(map[uint32]color.RGBA, options.colorCount)
	for y := canvas.Bounds().Min.Y; y < canvas.Bounds().Max.Y; y++ {
		for x := canvas.Bounds().Min.X; x < canvas.Bounds().Max.X; x++ {
			c := canvas.RGBAAt(x, y)
			c.A = 255
			colors[uint32(c.R)<<24|uint32(c.G)<<16|uint32(c.B)<<8|uint32(c.A)] = c
		}
	}
	palette := make(color.Palette, len(colors))
	i := 0
	for _, c := range colors {
		palette[i] = color.RGBA{R: c.R / 8 * 8, G: c.G / 8 * 8, B: c.B / 8 * 8, A: 255}
		i++
	}

	// sort the palette
	sort.Slice(palette, func(ai, bi int) bool {
		if palette[ai] == options.clearColor {
			// make sure the clear color is the first one
			return true
		}
		ar, ag, ab, _ := palette[ai].RGBA()
		br, bg, bb, _ := palette[bi].RGBA()
		asum := ar + ag + ab
		bsum := br + bg + bb
		return asum < bsum
	})

	paletted := image.NewPaletted(canvas.Bounds(), palette)
	// draw.Draw(paletted, canvas.Bounds(), canvas, image.Point{}, draw.Over)

	// bayer4 := []int16{
	// 	0, 8, 2, 10,
	// 	12, 4, 14, 6,
	// 	3, 11, 1, 9,
	// 	15, 7, 13, 5,
	// }

	bayer8 := []int16{
		0, 32, 8, 40, 2, 34, 10, 42,
		48, 16, 56, 24, 50, 18, 58, 26,
		12, 44, 4, 36, 14, 46, 6, 38,
		60, 28, 52, 20, 62, 30, 54, 22,
		3, 35, 11, 43, 1, 33, 9, 41,
		51, 19, 59, 27, 49, 17, 57, 25,
		15, 47, 7, 39, 13, 45, 5, 37,
		63, 31, 55, 23, 61, 29, 53, 21,
	}

	for y := canvas.Bounds().Min.Y; y < canvas.Bounds().Max.Y; y++ {
		for x := canvas.Bounds().Min.X; x < canvas.Bounds().Max.X; x++ {
			c := img.RGBAAt(x, y)

			if options.ditherIntensity != 0.0 {
				if !options.ditherClearColor && c.R == options.clearColor.R && c.G == options.clearColor.G && c.B == options.clearColor.B {
					continue
				}

				xi := x % 8
				yi := y % 8
				b := int16(float32(bayer8[yi*8+xi])*options.ditherIntensity + options.ditherAdd)

				if true {
					c.R = uint8(min(255, max(0, int16(c.R)+b)))
					c.G = uint8(min(255, max(0, int16(c.G)+b)))
					c.B = uint8(min(255, max(0, int16(c.B)+b)))
				}
			}

			paletted.Set(x, y, c)
		}
	}

	return paletted
}

func createMask(img *image.RGBA, threshold uint8) *image.Alpha {
	bounds := img.Bounds()
	newImg := image.NewAlpha(bounds)

	for y := bounds.Min.Y; y < bounds.Max.Y; y++ {
		for x := bounds.Min.X; x < bounds.Max.X; x++ {
			c := color.RGBAModel.Convert(img.At(x, y)).(color.RGBA)
			if c.R > threshold || c.G > threshold || c.B > threshold || c.A > threshold {
				newImg.Set(x, y, color.Opaque)
			} else {
				newImg.Set(x, y, color.Transparent)
			}
		}
	}

	return newImg
}

func loadImage(name string, width int, height int, mask_threshold uint8) (draw.Image, *image.Alpha, image.Rectangle, error) {

	bounds := image.Rectangle{}

	cwd, err := os.Getwd()
	if err != nil {
		return nil, nil, bounds, err
	}

	path := filepath.Join(cwd, fmt.Sprintf("../raw_assets/%s.png", name))
	content, err := os.Open(path)
	if err != nil {
		return nil, nil, bounds, err
	}
	defer content.Close()
	img, err := png.Decode(content)
	if err != nil {
		return nil, nil, bounds, err
	}

	scaled := image.NewRGBA(image.Rectangle{Max: image.Point{X: width, Y: height}})
	draw.BiLinear.Scale(scaled, scaled.Bounds(), img, img.Bounds(), draw.Over, nil)

	mask := createMask(scaled, mask_threshold)

	minX := scaled.Bounds().Dx()
	minY := scaled.Bounds().Dy()
	maxX := 0
	maxY := 0

	for y := scaled.Bounds().Min.Y; y < scaled.Bounds().Max.Y; y++ {
		for x := scaled.Bounds().Min.X; x < scaled.Bounds().Max.X; x++ {
			c := color.AlphaModel.Convert(scaled.At(x, y)).(color.Alpha)
			if c.A > mask_threshold {
				minX = min(x, minX)
				minY = min(y, minY)
				maxX = max(x, maxX)
				maxY = max(y, maxY)
			}
		}
	}

	bounds = image.Rectangle{
		Min: image.Point{minX, minY},
		Max: image.Point{maxX, maxY},
	}

	return scaled, mask, bounds, nil
}
