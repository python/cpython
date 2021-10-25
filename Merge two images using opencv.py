import cv2

img = cv2.imread('one.jpg')
img1 = cv2.imread('image 1.jpg')

print(img.shape)     # Tuple that gives no. of rows, columns & channels
print(img.size)      # Gives the total no. of pixels.
print(img.dtype)     # data type os image... it is 'uint8' here.

b,g,r=cv2.split(img)
img=cv2.merge((b,g,r))

#text=img[50:194,111:208]           # copy of text...
#img[263:24,325:38]=text             # pasting the image...

img = cv2.resize(img,(273,273))
img1 = cv2.resize(img1, (273,273))

# dst = cv2.add(img,img1)    # simple merge of 2 images...
dst = cv2.addWeighted(img,0.7,img1,0.3,0)
# here, 0.7 is the weight of img and 0.3 is the weight of img2
# gamma is 0 here, since no Scalar value added

cv2.imshow('image 1',img)
cv2.imshow('image 2',img1)
cv2.imshow('merged image',dst)
cv2.waitKey()
cv2.destroyAllWindows()
