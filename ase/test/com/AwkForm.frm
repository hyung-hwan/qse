VERSION 5.00
Begin VB.Form AwkForm 
   Caption         =   "ASE COM AWK"
   ClientHeight    =   7695
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   9435
   LinkTopic       =   "AwkForm"
   ScaleHeight     =   7695
   ScaleWidth      =   9435
   StartUpPosition =   3  'Windows Default
   Begin VB.TextBox ConsoleIn 
      BeginProperty Font 
         Name            =   "Courier New"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   2895
      Left            =   360
      MultiLine       =   -1  'True
      TabIndex        =   4
      Top             =   3600
      Width           =   4095
   End
   Begin VB.TextBox SourceIn 
      BeginProperty Font 
         Name            =   "Courier New"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   2775
      Left            =   480
      MultiLine       =   -1  'True
      TabIndex        =   3
      Top             =   480
      Width           =   3975
   End
   Begin VB.TextBox SourceOut 
      BeginProperty Font 
         Name            =   "Courier New"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   2775
      Left            =   4680
      MultiLine       =   -1  'True
      TabIndex        =   2
      Top             =   480
      Width           =   4095
   End
   Begin VB.CommandButton Execute 
      Caption         =   "Execute"
      Height          =   375
      Left            =   7080
      TabIndex        =   1
      Top             =   6840
      Width           =   1215
   End
   Begin VB.TextBox ConsoleOut 
      BeginProperty Font 
         Name            =   "Courier New"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   2895
      Left            =   4680
      MultiLine       =   -1  'True
      TabIndex        =   0
      Top             =   3600
      Width           =   4095
   End
End
Attribute VB_Name = "AwkForm"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Public WithEvents abc As ASELib.Awk
Attribute abc.VB_VarHelpID = -1
Private first As Boolean

Private Sub Execute_Click()
    Dim a As Long
    Dim x As Object
        
    first = True
    
    ConsoleOut.Text = ""
    SourceOut.Text = ""
    
    Set abc = New ASELib.Awk
    Call abc.Parse
    Call abc.Run
    Set abc = Nothing
End Sub

Function abc_OpenSource(ByVal mode As Long) As Long
    abc_OpenSource = 1
End Function

Function abc_CloseSource(ByVal mode As Long) As Long
    abc_CloseSource = 0
End Function

Function abc_ReadSource(ByVal buf As ASELib.Buffer) As Long
    If first Then
        buf.value = SourceIn.Text
        abc_ReadSource = Len(buf.value)
        first = False
    Else
        abc_ReadSource = 0
    End If
End Function

Function abc_WriteSource(ByVal buf As ASELib.Buffer) As Long
    Dim value As String, value2 As String, c As String
    Dim i As Integer, l As Integer
    
    value = buf.value
    If value = vbLf Then
        SourceOut.Text = SourceOut.Text + vbCrLf
        abc_WriteSource = 1
    Else
        l = Len(value)
        For i = 1 To l
            c = Mid(value, i, 1)
            If c = vbLf Then
                value2 = value2 + vbCrLf
            Else
                value2 = value2 + c
            End If
        Next i
        
        SourceOut.Text = SourceOut.Text + value2
        abc_WriteSource = l
    End If
    
End Function

Function abc_OpenExtio(ByVal extio As ASELib.AwkExtio) As Long
MsgBox "abc_OpenExtio"
    abc_OpenExtio = 1
End Function

Function abc_CloseExtio(ByVal extio As ASELib.AwkExtio) As Long
MsgBox "abc_CloseExtio"
    abc_CloseExtio = 0
End Function

Function abc_ReadExtio(ByVal extio As ASELib.AwkExtio, ByVal buf As ASELib.Buffer) As Long
    abc_ReadExtio = 0
End Function

Function abc_WriteExtio(ByVal extio As ASELib.AwkExtio, ByVal buf As ASELib.Buffer) As Long
    Dim value As String, i As Long, value2 As String
    
    value = buf.value
    
    'For i = 0 To 5000000
    '    value2 = "abc"
    '    buf.value = "abdkjsdfsafas"
    'Next i
    
    If value = vbLf Then
        ConsoleOut.Text = ConsoleOut.Text + vbCrLf
        abc_WriteExtio = 1
    Else
        ConsoleOut.Text = ConsoleOut.Text + value
        abc_WriteExtio = Len(value)
    End If
End Function

Private Sub Form_Load()
    SourceIn.Text = "BEGIN { print 123.12; print 995; print 5432.1; }"
    SourceOut.Text = ""
    ConsoleIn.Text = ""
    ConsoleOut.Text = ""
End Sub

