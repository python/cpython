"""
Make With EGYPTIAN PROGRAMMER in 2020
"""
import os
import uuid
import sqlite3
import random


# Dirs And Files Settings
def ft(name):
	"""(File Type)"""
	return str(name).split(".")[-1]
def cmd(command):os.system(command)
class o:
	"""[Open Files] Write AND Read Files\n/- ON Read -mode-> read variable -name-> o.rv\n/- ON Write -mode-> write variable -name-> o.wv"""

	def __init__(self, _file, _mode="r", _text=None, _lines=None):
		self.file_name=_file
		self.mode=_mode
		self.text=_text
		self.lines=_lines
		# work
		self.F = open(self.file_name, self.mode)
		if self.text!=None:self.wv = self.F.write(str(self.text))
		elif self.mode=="r":self.rv = self.F.read()
		if self.lines!=None:self.rv=self.F.readline(self.lines)
		self.F.close()

	def o(self):
		"""(open) ANY File TO Work"""
		self.F = open(self.file_name, self.mode)
	def c(self):
		"""(close) ANY File TO END Work"""
		self.F.close()

	def w(self, text):
		"""(write) ON ANY File"""
		self.o()
		self.wv = self.F.write(text)
		self.c()

	def r(self, lines=None):
		"""(read) From ANY File"""
		self.o()
		self.rv = self.F.read()
		if lines!=None:self.rv=self.F.readline(lines)
		self.c()


def e(name):
	"""[Exists] IF Dir And File Exists IN This Directory"""
	if os.path.exists(name):return True
	else:return False
try:
	def mk(name, location=os.getcwd()):
		"""[Make] Make Files And Dirs"""
		ch(location)
		os.mkdir(name)
	def ch(location):
		"""[Choose] Choose Dir TO Work"""
		os.chdir(location)
	def cmk(name, location=os.getcwd()):
		"""[Choose AND MaKe] Make Dir AND Open IT"""
		mk(name, location)
		ch(name)
	def cmka(name):
		"""(Choose AND MaKe File ON ANYWay)"""
		if not e(name):cmk(name)
		else:ch(name)
	def rn(old, new):
		"""(Rename) Dirs AND Files"""
		os.renames(old, new)
	def rm(name):
		"""(Remove) Dirs AND Files"""
		os.remove(name)
	def EC(old_name, old_ex,  new_ex):
		"""[Extnsion Convert] Convert ANY Extnsion For Any Files"""
		file = open(old_name+"."+old_ex, "r")
		read = file.read()
		file.close()
		rm(file)
		file = open(old_name+"."+new_ex, "w")
		file.write(read)
		file.close()
except FileNotFoundError:raise FileNotFoundError("File Not Found")
except FileExistsError:raise FileExistsError("File Aleady Exists")


# Show Infromations
def D(path=os.getcwd()):
	"""[Dir] Show Dirs And Files In This Directory"""
	for i in os.listdir(path):
		if os.path.isdir(i):print(f"<Dir>\t{i}")
		elif os.path.isfile(i):
			split=ft(i)
			if split=="py":split="python"
			if split=="txt":split="txt"
			if split in("gpj","e"):split="photo"
			print(f"<{split} File>\t{i}")
def P():
	"""[Path] Show Path For This Directory"""
	return os.getcwd()
def HN():
	"""[Host Name] Return Host Name o YOUR Windows"""
	return os.path.expanduser("~")


# Search Settings
def s(link):
	"""(Search) TO ANY Thing ON YOUR Defualt Browser"""
	os.system(f"python -m webbrowser -t \"{link}\"")


# Sound Settings
def tts(text=None, location=os.getcwd(), file_name=str(uuid.uuid4()), slow=False):
	"""[Text TO Speech] Convert Text To Speech"""
	try:
		ch(location)
		import gtts
	except FileNotFoundError:raise FileNotFoundError(f"lcoation {location} Not Found")
	except ImportError:
			print("gtts Module Requirerd. gtts is now loaded")
			cmd("pip install gtts")
			import gtts
	save_file = file_name+".mp3"
	output = gtts.gTTS(text=text, lang="en", slow=slow)
	try:output.save(save_file)
	except FileExistsError:raise FileExistsError(f"File {save_file} Already Exsists")
	os.startfile(save_file)


# Help Commands
def cls():
	"""(cls) Command From CMD"""
	os.system("cls")


# Git Language
class Git:
	"""Work With Git Language"""

	def __init__(self, _location=os.getcwd()):
		self.location = _location

	def p(self, commit_message="EGP", Branch="master"):
		"""(Push) Files TO Branch"""
		ch(self.location)
		if e(".git"):
			os.system("git add *")
			os.system(f"git commit -m {commit_message}")
			os.system(f"git push origin {Branch}")
		else:print("NOT Github Project In Here")

	def c(self, _link):
		self.link=_link
		"""(Clone) Files TO Work"""
		try:ch(self.location)
		except FileNotFoundError:
			print("Location NOT Found.")
			location = str(HN()+"/Documents")
			ch(location)
			print("clone Files In Documents Folder")
		os.system(f"git clone {self.link}")
		s(self.link)


# DataBase
class sqlite:
	"""[SQLite3] Work With SQLite"""

	def __init__(self, _file=uuid.uuid4()):
		self.file = _file+".db"
		self.i

	def i(self):
		try:import sqlite3
		except ImportError:
			print("SQLite Module Required. SQLite3 Loaded ..")
			cmd("pip install sqlite3")

	def o(self):
		"""(Create) SQLite File AND Connect"""
		self.db = sqlite3.connect(self.file)
		self.cr = self.db.cursor()

	def c(self):
		"""(Save) SQLite File AND Close Connect"""
		self.db.commit()
		self.db.close()


class Sign:
	"""Save AND Show Sign TO DataBase SQLite"""

	def __init__(self, name=None, age=None, email=None, password=None, repassword=None, _print_info=bool, _save_dir_=bool, _save_db_=bool, _save_txt_=bool, _save_py_=bool):
		self.name=name
		self.age=age
		self.email=email
		self.password=password
		self.repassword=repassword
		self.print__info=_print_info
		self.save__dir=_save_dir_
		self.save__db_=_save_db_
		self.save__txt=_save_txt_
		self.save__py=_save_py_
		# Work
		self.id_R()
		if self.name!=None:self.name_R()
		if self.age!=None:self.age_R()
		if self.email!=None:self.email_R()
		if self.password!=None:self.password_R()
		if self.repassword!=None:self.repassword_R()
		if self.print__info:self.P_info()
		if self.save__dir:cmka("Sign")
		if self.save__db_:self.s_db()
		if self.save__txt:self.s_txt()
		if self.save__py:self.s_py()


	def id_R(self):
		self.id=uuid.uuid4()


	def name_R(self):
		self.random_name=str(random.randint(0,10))+str(random.randint(10,100))+str(random.randint(100,1000))


	def age_R(self):
		if list(self.age)[0]=="0":raise ValueError("ANY age NOT Start With (0)")


	def email_R(self):
		if self.email!="user_email_none":
			if "@gmail.com" not in self.email:self.email +="@gmail.com"
			else:self.email=self.email


	def password_R(self):
		self.password_words_random = ["a","b","c","d","e","f","g","h","i","g","k","l","m","n","o","p","q","r","s","t","u","v","w","x","y","z"]
		self.random_password = str(str(random.randint(0,10))+ self.password_words_random[random.randint(0,10)]+str(random.randint(10,100))+ self.password_words_random[random.randint(10,19)]+ self.password_words_random[random.randint(13,len(self.password_words_random)-1)]+str(random.randint(100,1000)))


	def repassword_R(self):
		if self.repassword==self.password:print("correct.")
		else:raise ValueError("repassword IS NOT EQUAL password")


	def P_info(self):
		print("id:",self.id)
		print("name:",self.name)
		print("age:",self.age)
		print("email:",self.email)
		print("password:",self.password)
		print("repassword:",self.repassword)

	def s_txt(self):o("sign_info.txt","a",f"ID:{self.id}\nName:{self.name}\nAge:{self.age}\nEmail:{self.email}\nPassword:{self.password}\n-------------------------\n")
	def s_db(self):
		sql = sqlite("sign")
		sql.o()
		sql.cr.execute("CREATE TABLE IF NOT EXISTS sign(id text, Name text, Age integer, Email text, Password text)")
		sql.cr.execute(f"INSERT INTO sign(id, Name, Age, Email, Password) values('{self.id}', '{self.name}', '{self.age}', '{self.email}', '{self.password}')")
		sql.c()
	def s_py(self):
		o("sign_info.py","a",f"id='{self.id}'\nname='{self.name}'\nage='{self.age}'\nemail='{self.email}'\npassword='{self.password}'\n# --------------------------\n")


# Easy Modules
class CTK:
	"""Create Tkinter App"""

	def __init__(self, title=None):
		self.title = title
		self.wedgets_info = ["l", "b", "e", "mb"]
		# Work
		cmka("CTK")
		self.o=o(self.title+".py", "a",f"""\"\"\"\nMake With Egyptian Programmer\n\"\"\"\nfrom tkinter import *\nfrom tkinter.ttk import *\nfrom tkinter import messagebox\n\nroot = Tk()\nroot.title = '{self.title}'\n""")

	def c(self):
		"""(close) Project"""
		self.o.w("""\n# Mainloop\nroot.mainloop()\n""")
		os.startfile(f"{self.title+'.py'}")
		ch(os.getcwd())

	def w(self, widget, x=None, y=None, width=None, height=None, text=None, message=None):
		self.wedget = widget
		self.text = text
		if self.wedget in self.wedgets_info:
			self.iswedget = True
			self.X = x
			self.Y = y
			if self.wedget in ("b", "e"):
				self.Waxis = width
				self.Haxis = height
		else:
			self.iswedget = False
			print("NOT Wedget")
			self.w(self.wedget, self.X, self.Y)
		if self.wedget == "l":
			self.o.w(f"""{self.text} = Label(root, text='{self.text}').place(x={self.X}, y={self.Y})\n""")

		if self.wedget == "b":
			self.o.w(f"""{self.text} = Button(root, text='{self.text}').place(width={self.Waxis}, height={self.Haxis}, x={self.X}, y={self.Y})\n""")

		if self.wedget == "e":
			self.o.w(f"""{self.text} = Entry(root, width={self.Waxis}).place(height={self.Haxis}, x={self.X}, y={self.Y})\n""")
		if self.wedget == "mb":self.mb(self.text, message)

	def mb(self, title=None, message=None):
			self.o.w(f"""{self.text} = messagebox.showinfo('{title}','{message}')\n""")


# Admin Settings
class Admin:

	def __init__(self, _name, _password):
		self.name = _name
		self.password = _password
		self.admin = False

	def a_i(self):
		self.test_name = input("name: ")
		self.test_password = input("password: ")
		if self.test_name == self.name and self.test_password == self.password:self.admin = True
		else:self.admin = False
