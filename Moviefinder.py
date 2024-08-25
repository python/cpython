# Made by Ajinesh Pratap Singh.....

import tkinter as tk
from tkinter import messagebox
from imdb import IMDb

# Function to search for the movie and display the details
def search_movie():
    movie_title = entry.get()
    if not movie_title:
        messagebox.showwarning("Input Error", "Please enter a movie name.")
        return
    
    movies = ia.search_movie(movie_title)
    
    if movies:
        movie = movies[0]
        ia.update(movie)
        
        # Display movie details
        movie_details = f"Title: {movie['title']}\n"
        movie_details += f"Year: {movie['year']}\n"
        movie_details += f"Rating: {movie.get('rating')}\n"
        movie_details += f"Genres: {', '.join(movie.get('genres', []))}\n"
        movie_details += f"Plot: {movie.get('plot outline', 'N/A')}\n"
        
        # Fetch box office information
        box_office = movie.get('box office')
        if box_office:
            worldwide_gross = box_office.get('Cumulative Worldwide Gross', 'Not available')
            usa_gross = box_office.get('Gross USA', 'Not available')
            movie_details += f"Worldwide Gross: {worldwide_gross}\n"
            movie_details += f"USA Gross: {usa_gross}\n"
        else:
            movie_details += "Box office information not available."
        
        # Update the text area with movie details
        text_area.config(state=tk.NORMAL)
        text_area.delete(1.0, tk.END)
        text_area.insert(tk.END, movie_details)
        text_area.config(state=tk.DISABLED)
    else:
        messagebox.showinfo("No Results", "No movies found.")

# Initialize IMDbPY
ia = IMDb()

# Create the main application window
root = tk.Tk()
root.title("Movie Search")

# Create and place the widgets
label = tk.Label(root, text="Enter Movie Name:")
label.pack(pady=10)

entry = tk.Entry(root, width=40)
entry.pack(pady=5)

search_button = tk.Button(root, text="Search", command=search_movie)
search_button.pack(pady=10)

text_area = tk.Text(root, wrap=tk.WORD, state=tk.DISABLED, width=60, height=15)
text_area.pack(padx=10, pady=10)

# Run the main event loop
root.mainloop()
