![077](https://github.com/user-attachments/assets/bbda4c85-877e-40be-b051-378bd1ede930) 
import tkinter as tk
from tkinter import colorchooser

def mix_colors():
    # اختيار اللون الأول
    color1 = colorchooser.askcolor(title="اختر اللون الأول")
    # اختيار اللون الثاني
    color2 = colorchooser.askcolor(title="اختر اللون الثاني")
    
    if color1[1] and color2[1]:  # التأكد من أن المستخدم لم يلغِ اختيار الألوان
        # تحويل الألوان من HEX إلى RGB
        r1, g1, b1 = int(color1[0][0]), int(color1[0][1]), int(color1[0][2])
        r2, g2, b2 = int(color2[0][0]), int(color2[0][1]), int(color2[0][2])
        
        # دمج الألوان (متوسط القيم)
        mixed_r = (r1 + r2) // 2
        mixed_g = (g1 + g2) // 2
        mixed_b = (b1 + b2) // 2
        
        # عرض اللون المدمج
        mixed_color = f'#{mixed_r:02x}{mixed_g:02x}{mixed_b:02x}'
        color_label.config(bg=mixed_color)
        color_label.config(text=f"اللون المدمج: {mixed_color}")

# إنشاء النافذة الرئيسية
root = tk.Tk()
root.title("دمج الألوان")

# إنشاء زر لاختيار الألوان ودمجها
mix_button = tk.Button(root, text="دمج الألوان", command=mix_colors)
mix_button.pack(pady=20)

# عرض اللون المدمج
color_label = tk.Label(root, text="اللون المدمج سيظهر هنا", font=("Arial", 14), width=30, height=10)
color_label.pack(pady=20)

# بدء تشغيل الواجهة
root.mainloop() 
