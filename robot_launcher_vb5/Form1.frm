VERSION 5.00
Begin VB.Form Form1 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "Launcher"
   ClientHeight    =   3030
   ClientLeft      =   45
   ClientTop       =   375
   ClientWidth     =   4560
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   3030
   ScaleWidth      =   4560
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton Command1 
      Caption         =   "Start"
      Height          =   495
      Left            =   1800
      TabIndex        =   4
      Top             =   2520
      Width           =   855
   End
   Begin VB.FileListBox File2 
      Height          =   870
      Left            =   240
      Pattern         =   "*.dll"
      TabIndex        =   3
      Top             =   1560
      Width           =   4095
   End
   Begin VB.FileListBox File1 
      Height          =   870
      Left            =   240
      Pattern         =   "*.txt"
      TabIndex        =   0
      Top             =   360
      Width           =   4095
   End
   Begin VB.Label Label2 
      Caption         =   "Robots"
      Height          =   255
      Left            =   240
      TabIndex        =   2
      Top             =   1320
      Width           =   1815
   End
   Begin VB.Label Label1 
      Caption         =   "Maps:"
      Height          =   255
      Left            =   240
      TabIndex        =   1
      Top             =   120
      Width           =   1575
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Private Sub Command1_Click()
Shell App.Path + "\robocleaner.exe " + Chr(34) + "maps\" + File1.filename + Chr(34) + " " + Chr(34) + "robots\" + File2.filename + Chr(34)
End Sub

Private Sub Form_Load()
    File1.Path = App.Path & "\maps"
    File2.Path = App.Path & "\robots"
End Sub
