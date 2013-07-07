NSImage* NSMakeImage(nall::image image, unsigned width = 0, unsigned height = 0) {
  if(image.empty()) return nil;
  if(width && height) image.scale(width, height, Interpolation::Linear);
  image.transform(0, 32, 255u << 24, 255u << 0, 255u << 8, 255u << 16);
  NSImage *cocoaImage = [[[NSImage alloc] initWithSize:NSMakeSize(image.width, image.height)] autorelease];
  NSBitmapImageRep *bitmap = [[[NSBitmapImageRep alloc]
    initWithBitmapDataPlanes:nil
    pixelsWide:image.width pixelsHigh:image.height
    bitsPerSample:8 samplesPerPixel:4 hasAlpha:YES
    isPlanar:NO colorSpaceName:NSCalibratedRGBColorSpace
    bitmapFormat:NSAlphaNonpremultipliedBitmapFormat
    bytesPerRow:image.pitch bitsPerPixel:32
  ] autorelease];
  memcpy([bitmap bitmapData], image.data, image.height * image.pitch);
  [cocoaImage addRepresentation:bitmap];
  return cocoaImage;
}
