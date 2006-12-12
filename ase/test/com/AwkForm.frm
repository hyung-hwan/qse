VERSION 5.00
Begin VB.Form AwkForm 
   Caption         =   "ASE COM AWK"
   ClientHeight    =   7770
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   10335
   LinkTopic       =   "AwkForm"
   ScaleHeight     =   7770
   ScaleWidth      =   10335
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
      Left            =   120
      MultiLine       =   -1  'True
      TabIndex        =   4
      Top             =   3600
      Width           =   5055
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
      Left            =   120
      MultiLine       =   -1  'True
      TabIndex        =   3
      Top             =   480
      Width           =   5055
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
      Left            =   5280
      MultiLine       =   -1  'True
      TabIndex        =   2
      Top             =   480
      Width           =   4935
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
      Left            =   5280
      MultiLine       =   -1  'True
      TabIndex        =   0
      Top             =   3600
      Width           =   4935
   End
End
Attribute VB_Name = "AwkForm"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Public WithEvents Awk As ASELib.Awk
Attribute Awk.VB_VarHelpID = -1
Private first As Boolean

Private extio_first As Boolean

Private Sub Execute_Click()
    first = True
    
    ConsoleOut.Text = ""
    SourceOut.Text = ""
    
    Set Awk = New ASELib.Awk
    Awk.Option = Awk.Option Or ASELib.AWK_SHADING Or ASELib.AWK_IDIV
    
    If Awk.Parse() = -1 Then
        MsgBox "PARSE ERROR OCCURRED!!!"
    End If
    If Awk.Run() = -1 Then
        MsgBox "RUN ERROR OCCURRED!!!"
    End If
    Set Awk = Nothing
    
End Sub

Function Awk_OpenSource(ByVal mode As Long) As Long
    Awk_OpenSource = 1
End Function

Function Awk_CloseSource(ByVal mode As Long) As Long
    Awk_CloseSource = 0
End Function

Function Awk_ReadSource(ByVal buf As ASELib.Buffer) As Long
    If first Then
        buf.value = SourceIn.Text
        Awk_ReadSource = Len(buf.value)
        first = False
    Else
        Awk_ReadSource = 0
    End If
End Function

Function Awk_WriteSource(ByVal buf As ASELib.Buffer) As Long
    Dim value As String, value2 As String, c As String
    Dim i As Integer, l As Integer
    
    value = buf.value
    If value = vbLf Then
        SourceOut.Text = SourceOut.Text + vbCrLf
        Awk_WriteSource = 1
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
        Awk_WriteSource = l
    End If
    
End Function

Function Awk_OpenExtio(ByVal extio As ASELib.AwkExtio) As Long

    Dim console As AwkExtioConsole
    Awk_OpenExtio = -1
    
    Select Case extio.Type
    Case ASELib.AWK_EXTIO_CONSOLE
        If extio.mode = ASELib.AWK_EXTIO_CONSOLE_READ Then
            extio_first = True
            extio.Handle = 1234
            Awk_OpenExtio = 1
        ElseIf extio.mode = ASELib.AWK_EXTIO_CONSOLE_WRITE Then
            extio_first = True
            Set console = New AwkExtioConsole
            console.Active = True
            console.Count = 0
            Set extio.Handle = console
            Awk_OpenExtio = 1
        End If
    Case ASELib.AWK_EXTIO_FILE
    Case ASELib.AWK_EXTIO_PIPE
    Case ASELib.AWK_EXTIO_COPROC
    End Select
    
End Function

Function Awk_CloseExtio(ByVal extio As ASELib.AwkExtio) As Long

    Awk_CloseExtio = -1
    
    Select Case extio.Type
    Case ASELib.AWK_EXTIO_CONSOLE
        If extio.mode = ASELib.AWK_EXTIO_CONSOLE_READ Then
            Awk_CloseExtio = 0
        ElseIf extio.mode = ASELib.AWK_EXTIO_CONSOLE_WRITE Then
            Awk_CloseExtio = 0
        End If
    Case ASELib.AWK_EXTIO_FILE
    Case ASELib.AWK_EXTIO_PIPE
    Case ASELib.AWK_EXTIO_COPROC
    End Select
    
End Function

Function Awk_ReadExtio(ByVal extio As ASELib.AwkExtio, ByVal buf As ASELib.Buffer) As Long
    Dim console As AwkExtioConsole
    Dim value As String, value2 As String
    Dim l As Integer, i As Integer
    
    If extio.mode <> 0 Then
        Awk_ReadExtio = -1
        Exit Function
    End If
        
    Set console = extio.Handle
    If console.Count = 0 Then
        value = ConsoleIn.Text
        l = Len(value)
        
        For i = 1 To l - 1
            If Mid(value, i, 2) = vbCrLf Then
                value2 = value2 + vbLf
                i = i + 1
            Else
                value2 = value2 + Mid(value, i, 1)
            End If
        Next
        If i = l Then
            value2 = value2 + Mid(value, i, 1)
        End If
        
        console.Count = console.Count + 1
        buf.value = value2
        Awk_ReadExtio = Len(value2)
    Else
        Awk_ReadExtio = 0
    End If
End Function

Function Awk_WriteExtio(ByVal extio As ASELib.AwkExtio, ByVal buf As ASELib.Buffer) As Long
    Dim value As String, i As Long, value2 As String
    
    If extio.mode <> 1 Then
        Awk_WriteExtio = -1
        Exit Function
    End If
        
    value = buf.value
    
    'For i = 0 To 5000000
    '    value2 = "abc"
    '    buf.value = "abdkjsdfsafas"
    'Next i
    
    If value = vbLf Then
        ConsoleOut.Text = ConsoleOut.Text + vbCrLf
        Awk_WriteExtio = 1
    Else
        ConsoleOut.Text = ConsoleOut.Text + value
        Awk_WriteExtio = Len(value)
    End If
End Function

Private Sub Form_Load()
    SourceIn.Text = "BEGIN { print 123.12; print 995; print 5432.1; }"
    SourceOut.Text = ""
    ConsoleIn.Text = ""
    ConsoleOut.Text = ""
End Sub

