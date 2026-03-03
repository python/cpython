gh-145417: Fixed venv to prevent incorrect preservation of SELinux contexts when copying scripts by using shutil.copy instead of shutil.copy2.
