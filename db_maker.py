def generate_student_database():
    with open("database.txt", "w") as f:
        for i in range(1, 21):  # 20 students
            if i == 20:
                f.write("9999\n")  # End marker
            else:
                f.write(f"{i:04d}\n")

if __name__ == "__main__":
    generate_student_database()